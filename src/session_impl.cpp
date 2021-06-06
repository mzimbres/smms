/* Copyright (c) 2018-2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This file is part of smms.
 *
 * smms is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * smms is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with smms.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "session_impl.hpp"

#include <iterator>
#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include "crypto.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace fs = boost::filesystem;

namespace smms {

beast::string_view
config::get_cache_control(beast::string_view path) const noexcept
{
   auto ext = make_extension(path);

   auto f = [&](beast::string_view const& mime)
     { return beast::iequals(ext, mime); };

   auto match =
     std::find_if(std::cbegin(local_cache_control_mime),
		  std::cend(local_cache_control_mime),
		  f);

   if (match == std::end(local_cache_control_mime))
     return default_cache_control;

   auto d = std::distance(std::cbegin(local_cache_control_mime), match);
   return local_cache_control_value[d];
}

http::response<http::string_body>
make_post_response(
   beast::string_view raw_target,
   http::request_parser<http::string_body> const& parser,
   config const& cfg)
{
   http::response<http::string_body> response;

   std::string path;
   auto const target_query = split_from_query(raw_target);
   auto const target = target_query.first;
   auto const query = target_query.second;
   auto const queries = parse_query({query.data(), std::size(query)});
   
   auto const expected_hex_auth = get_field_value(queries, "hmac");

   if (expected_hex_auth.empty()) {
      response.result(http::status::bad_request);
      response.set(http::field::content_type, "text/plain");
      response.body() = "hmacsha256 is empty.\r\n";
      response.set(http::field::content_length,
		    beast::to_static_string(std::size(response.body())));
      return response;
   }

   hmacsha256::auth_type expected_auth = {{0}};

   auto const r =
      sodium_hex2bin(
	 expected_auth.data(),
	 expected_auth.size(),
	 expected_hex_auth.data(),
	 expected_hex_auth.size(),
	 nullptr,
	 nullptr,
	 nullptr);

   if (r == -1) {
      response.result(http::status::bad_request);
      response.set(http::field::content_type, "text/plain");
      response.body() = "Invalid hmacsha256.\r\n";
      response.set(http::field::content_length,
		    beast::to_static_string(std::size(response.body())));
      return response;
   }

   auto const in = std::string{target.data(), std::size(target)};
   auto const auth = hmacsha256::make_auth(in, cfg.key);

   // Before posting we check if the digest and the rest of the
   // target have been produced by the same key.
   if (auth == expected_auth) {
      path = cfg.doc_root;
      path.append(target.data(), std::size(target));

      std::string full_dir;
      full_dir += cfg.doc_root;
      full_dir += "/";
      full_dir += parse_dir({target.data(), std::size(target)});

      create_dir(full_dir.data());
   }

   log::write(
      log::level::debug,
      "make_post_response: target: {0}",
      path);

   if (std::empty(path)) {
      response.result(http::status::bad_request);
      response.set(http::field::content_type, "text/plain");
      response.body() = "Invalid signature.\r\n";
      response.set(http::field::content_length,
		    beast::to_static_string(std::size(response.body())));
      return response;
   }

   log::write(
      log::level::debug,
      "make_post_response: body size: {0}.",
      std::size(parser.get().body()));

   std::ofstream ofs(path, std::ios::binary);
   if (!ofs) {
      log::write(
	 log::level::info,
	 "make_post_response: Can't open file for writing.");

      response.result(http::status::bad_request);
      response.set(http::field::content_type, "text/plain");
      response.body() = "Error\r\n";
      response.set(http::field::content_length,
		beast::to_static_string(std::size(response.body())));
      return response;
   }

   ofs << parser.get().body();

   response.result(http::status::ok);
   response.set(http::field::server, cfg.server_name);
   response.set(http::field::access_control_allow_origin,
	     cfg.allow_origin);

   response.set(http::field::content_length,
	     beast::to_static_string(std::size(response.body())));
   return response;
}

http::response<http::string_body>
make_get_response(
   beast::string_view raw_target,
   http::request_parser<http::string_body> const& parser,
   config const& cfg)
{
   http::response<http::string_body> response;

   log::write(
      log::level::debug,
      "get_handler: target: {0}",
      raw_target);

   auto const target_query = split_from_query(raw_target);
   auto const target = target_query.first;
   assert(!std::empty(target));

   auto path = cfg.doc_root;
   path.append(target.data(), std::size(target));
   if (std::size(target) == 1)
      path += cfg.default_file;

   auto final_path = path;
   auto gzip = false;
   if (has_mime(path, cfg.gzip_mimes)) {
     // We must be able to server this mime type with
     // pre-compressed files. That means we have to add a ".gz" to
     // the path but only if the client accepts that encoding. If
     // it doesn't we do not change the filename.
     auto const match =
	parser.get().find(http::field::accept_encoding);

     if (match != std::end(parser.get())) {
	if (match->value().find("gzip") != std::string::npos) {
	  // check whether gzip version exists.
	  if (fs::exists(fs::path(final_path + ".gz"))) {
	    final_path += ".gz";
	    gzip = true;
	  }
	}
     }
   }

   log::write(
      log::level::debug,
      "get_handler: target (final): {0}",
      final_path);

   std::ifstream ifs{final_path};
   if (!ifs) {
      log::write(log::level::debug, "get_handler: Can't open file.");
      response.result(http::status::not_found);
      response.set(http::field::content_type, mime_type(".txt"));
      response.body() = "File not found.\r\n";
      response.set(http::field::content_length,
		beast::to_static_string(std::size(response.body())));
      return response;
   }

   std::string body;

   auto const is_jpeg = make_extension(path) == ".jpeg";
   auto const is_jpg = make_extension(path) == ".jpg";

   auto const query = target_query.second;
   auto const queries = parse_query({query.data(), std::size(query)});
   auto const width_str = get_field_value(queries, "width");
   auto const height_str = get_field_value(queries, "height");
   auto const is_img_query = !std::empty(width_str) && !std::empty(height_str);

   if ((is_jpeg || is_jpg) && is_img_query) {
      auto wec = error_code::ok;
      auto const width = stoi_nothrow(width_str, wec);

      auto hec = error_code::ok;
      auto const height = stoi_nothrow(height_str, hec);

      if (wec == error_code::ok && hec == error_code::ok) {
	 auto const ok_sizes_w = width <= 1000 && width > 0;
	 auto const ok_sizes_h = height <= 1000 && height > 0;
	 if (ok_sizes_w && ok_sizes_h) {
	    namespace bg = boost::gil;
	    bg::rgb8_image_t img;
	    bg::read_image(ifs, img, bg::jpeg_tag{});
	    bg::rgb8_image_t square(width, height);
	    bg::resize_view(
	       bg::const_view(img),
	       bg::view(square),
	       bg::bilinear_sampler{});

	    std::ostringstream oss;
	    bg::write_view(oss, bg::const_view(square), bg::jpeg_tag{});
	    body = oss.str();
	 } else {
	    response.result(http::status::bad_request);
	    response.set(http::field::content_type, mime_type(".txt"));
	    response.body() = "Invalid size.\r\n";
	    response.set(http::field::content_length,
	       	 beast::to_static_string(std::size(response.body())));
	    return response;
	 }
      } else {
         response.result(http::status::bad_request);
         response.set(http::field::content_type, mime_type(".txt"));
         response.body() = "Invalid query.\r\n";
         response.set(http::field::content_length,
       	      beast::to_static_string(std::size(response.body())));
         return response;
      }
   } else {
      using iter_type = std::istreambuf_iterator<char>;
      body = std::string {iter_type {ifs}, {}};
   }

   auto const body_size = std::size(body);
   response.body() = std::move(body);
   response.set(http::field::server, cfg.server_name);
   response.set(http::field::content_type, mime_type(path));
   response.content_length(body_size);
   response.keep_alive(parser.keep_alive());
   response.set(http::field::access_control_allow_origin,
		 cfg.allow_origin);
   if (gzip)
     response.set(http::field::content_encoding, "gzip");

   if (cfg.set_cache_control()) {
     response.set(
	http::field::cache_control,
	cfg.get_cache_control(path));
   }

   return response;
}

http::response<http::string_body>
make_redirect_response(beast::string_view target, config const& cfg)
{
   log::write(log::level::debug,
	      "make_redirect_response: redirecting to {0}",
	      cfg.redirect_url);

   http::response<http::string_body> response;
   response.result(http::status::moved_permanently);
   auto url = cfg.redirect_url;
   url.append(target.data(), std::size(target));
   response.set(http::field::location, url);
   response.set(http::field::content_length, beast::to_static_string(0));
   response.set(http::field::access_control_allow_origin, cfg.allow_origin);
   return response;
}

http::response<http::string_body>
make_response(
   http::request_parser<http::string_body> const& parser,
   config const& cfg,
   bool is_ssl)
{
   http::response<http::string_body> response;

   if (!log::ignore(log::level::debug)) { // Optimization.
      for (auto const& field : parser.get()) {
	 log::write(
	    log::level::debug,
	    "   {0}: {1}",
	    field.name_string(),
	    field.value());
      }
   }

   auto const target = parser.get().target();
   auto const match = parser.get().find(http::field::host);
   auto no_host_match = false;
   if (match != std::end(parser.get())) {
      auto const v = match->value();
      auto const n =
	 std::count(std::begin(cfg.host_names),
		    std::end(cfg.host_names),
		    std::string {v.data(), std::size(v)});
      no_host_match = n == 0;
   }

   auto const empty_redir_url = std::empty(cfg.redirect_url);
   if (no_host_match || !(empty_redir_url || is_ssl))
      return make_redirect_response(target, cfg);

   response.version(parser.get().version());
   response.keep_alive(false);

   switch (parser.get().method()) {
      case http::verb::post: return make_post_response(target, parser, cfg);
      case http::verb::get: return make_get_response(target, parser, cfg);
      default:
      {
	 response.result(http::status::bad_request);
	 response.set(http::field::content_type, "text/plain");
	 response.set(http::field::content_length,
		      beast::to_static_string(std::size(response.body())));
	 return response;
      } break;
   }
}

} // smms
