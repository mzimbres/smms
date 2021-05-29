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

#pragma once

#include "net.hpp"

#include <array>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <boost/filesystem.hpp>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include "utils.hpp"
#include "logger.hpp"
#include "crypto.hpp"

namespace fs = boost::filesystem;

namespace smms
{

struct config {
   std::vector<std::string> host_names;
   std::vector<std::string> gzip_mimes;

   std::vector<std::string> local_cache_control_mime;
   std::vector<std::string> local_cache_control_value;

   std::string server_name;
   std::string redirect_url;
   std::string doc_root;
   std::string key;
   std::string allow_origin;
   std::string default_file;
   std::string default_cache_control;
   std::uint64_t body_limit {1000000}; 
   int http_session_timeout {30};

   auto set_cache_control() const noexcept
      { return !std::empty(default_cache_control);}

   beast::string_view
   get_cache_control(beast::string_view path) const noexcept
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
};

template<class Derived>
class session {
protected:
   beast::flat_buffer buffer_ {8192};

private:
   http::request_parser<http::string_body> parser_;
   http::response<http::string_body> response_;
   
   config const& cfg_;

   Derived& derived()
      { return static_cast<Derived&>(*this); }

   void post_handler(beast::string_view raw_target)
   {
      std::string path;
      auto const target_query = split_from_query(raw_target);
      auto const target = target_query.first;
      auto const query = target_query.second;
      auto const queries = parse_query({query.data(), std::size(query)});
      
      auto const expected_hmac = get_field_value(queries, "hmac");
      auto const digest =
	 make_hex_digest({target.data(), std::size(target)}, cfg_.key);

      // Before posting we check if the digest and the rest of the
      // target have been produced by the same key.
      if (!std::empty(digest) && digest == expected_hmac) {
	 path = cfg_.doc_root;
	 path.append(target.data(), std::size(target));

	 log::write(log::level::debug , "post_handler: dir: {0}", path);

	 std::string full_dir;
	 full_dir += cfg_.doc_root;
	 full_dir += "/";
	 full_dir += parse_dir({target.data(), std::size(target)});

	 log::write(log::level::debug , "post_handler: mms dir: {0}", full_dir);
	 create_dir(full_dir.data());
      }

      log::write(
	 log::level::debug,
	 "post_handler: full dir: {0}",
	 path);

      if (std::empty(path)) {
	 response_.result(http::status::bad_request);
	 response_.set(http::field::content_type, "text/plain");
	 response_.body() = "Invalid signature.\r\n";
         response_.set(http::field::content_length,
                       beast::to_static_string(std::size(response_.body())));
	 write_response();
	 return;
      }

      log::write(
	 log::level::debug,
	 "post_handler: body size: {0}.",
	 std::size(parser_.get().body()));

      std::ofstream ofs(path, std::ios::binary);
      if (!ofs) {
         log::write(
	    log::level::info,
	    "post_handler: Can't open file for writing.");

         response_.result(http::status::bad_request);
         response_.set(http::field::content_type, "text/plain");
         response_.body() = "Error\r\n";
         response_.set(http::field::content_length,
                   beast::to_static_string(std::size(response_.body())));
         write_response();
         return;
      }

      ofs << parser_.get().body();

      response_.result(http::status::ok);
      response_.set(http::field::server, cfg_.server_name);
      response_.set(http::field::access_control_allow_origin,
		cfg_.allow_origin);

      response_.set(http::field::content_length,
                beast::to_static_string(std::size(response_.body())));

      write_response();
   }

   void get_handler(beast::string_view raw_target)
   {
      log::write(
	 log::level::debug,
	 "get_handler: target: {0}",
	 raw_target);

      auto const target_query = split_from_query(raw_target);
      auto const target = target_query.first;
      assert(!std::empty(target));

      auto path = cfg_.doc_root;
      path.append(target.data(), std::size(target));
      if (std::size(target) == 1)
	 path += cfg_.default_file;

      auto final_path = path;
      auto gzip = false;
      if (has_mime(path, cfg_.gzip_mimes)) {
	// We must be able to server this mime type with
	// pre-compressed files. That means we have to add a ".gz" to
	// the path but only if the client accepts that encoding. If
	// it doesn't we do not change the filename.
        auto const match =
	   parser_.get().find(http::field::accept_encoding);

        if (match != std::end(parser_.get())) {
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
	 response_.result(http::status::not_found);
	 response_.set(http::field::content_type, mime_type(".txt"));
	 response_.body() = "File not found.\r\n";
         response_.set(http::field::content_length,
                   beast::to_static_string(std::size(response_.body())));
	 write_response();
	 return;
      }

      std::string body;
      auto const is_jpeg = make_extension(path) == ".jpeg";
      auto const is_jpg = make_extension(path) == ".jpg";
      auto const query = target_query.second;
      if ((is_jpeg || is_jpg) && !std::empty(query)) {
	  auto const queries = parse_query({query.data(), std::size(query)});
	  auto const width_str = get_field_value(queries, "width");
	  auto const height_str = get_field_value(queries, "height");

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
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, mime_type(".txt"));
		response_.body() = "Invalid size.\r\n";
		response_.set(http::field::content_length,
			    beast::to_static_string(std::size(response_.body())));
		write_response();
	        return;
	     }
	  } else {
	     // TODO: Set error in the response.
	     response_.result(http::status::bad_request);
	     response_.set(http::field::content_type, mime_type(".txt"));
	     response_.body() = "Invalid query.\r\n";
	     response_.set(http::field::content_length,
			 beast::to_static_string(std::size(response_.body())));
	     write_response();
	     return;
	  }
      } else {
	 using iter_type = std::istreambuf_iterator<char>;
	 body = std::string {iter_type {ifs}, {}};
      }

      auto const body_size = std::size(body);
      response_.body() = std::move(body);
      response_.set(http::field::server, cfg_.server_name);
      response_.set(http::field::content_type, mime_type(path));
      response_.content_length(body_size);
      response_.keep_alive(parser_.keep_alive());
      response_.set(http::field::access_control_allow_origin,
                    cfg_.allow_origin);
      if (gzip)
        response_.set(http::field::content_encoding, "gzip");

      if (cfg_.set_cache_control()) {
        response_.set(
           http::field::cache_control,
           cfg_.get_cache_control(path));
      }

      write_response();
   }

   void set_redirect(beast::string_view target)
   {
      log::write(log::level::debug,
		 "set_redirect: redirecting to {0}",
		 cfg_.redirect_url);

      response_.result(http::status::moved_permanently);
      std::string url = cfg_.redirect_url;
      url.append(target.data(), std::size(target));
      response_.set(http::field::location, url);
      response_.set(http::field::content_length, beast::to_static_string(0));
      response_.set(http::field::access_control_allow_origin,
			   cfg_.allow_origin);
   }

   void on_read(boost::system::error_code ec, std::size_t n)
   {
      log::write(
	 log::level::debug,
	 "on_read: number of bytes read {0}.",
	 n);

      if (!log::ignore(log::level::debug)) { // Optimization.
	 for (auto const& field : parser_.get()) {
	    log::write(
	       log::level::debug,
	       "   {0}: {1}",
	       field.name_string(),
	       field.value());
	 }
      }

      if (ec) {
	 response_.result(http::status::bad_request);
	 response_.set(http::field::content_type, "text/plain");
	 response_.body() = "Invalid body size.\r\n";
         response_.set(http::field::content_length,
                       beast::to_static_string(std::size(response_.body())));
	 write_response();
	 return;
      }

      auto const target = parser_.get().target();
      auto const match = parser_.get().find(http::field::host);
      auto no_host_match = false;
      if (match != std::end(parser_.get())) {
	 auto const v = match->value();
	 auto const n =
	    std::count(std::begin(cfg_.host_names),
	               std::end(cfg_.host_names),
	               std::string {v.data(), std::size(v)});
	 no_host_match = n == 0;
      }

      auto const empty_redir_url = std::empty(cfg_.redirect_url);
      auto const is_ssl = derived().is_ssl();

      if (no_host_match || !(empty_redir_url || is_ssl)) {
	 set_redirect(target);
	 write_response();
	 return;
      }

      response_.version(parser_.get().version());
      response_.keep_alive(false);

      switch (parser_.get().method()) {
	 case http::verb::post: post_handler(target); break;
	 case http::verb::get: get_handler(target); break;
	 default:
	 {
	    response_.result(http::status::bad_request);
	    response_.set(http::field::content_type, "text/plain");
	    response_.set(http::field::content_length,
		      beast::to_static_string(std::size(response_.body())));
	    write_response();
	 } break;
      }
   }

   void write_response()
   {
      auto self = derived().shared_from_this();
      auto f = [self](auto ec, auto)
	 { self->derived().do_eof(); };

      http::async_write(derived().stream(), response_, f);
   }

public:
   session(config const& arg, beast::flat_buffer buffer)
   : cfg_ {arg}
   , buffer_(std::move(buffer))
   { }

   void do_read()
   {
      auto const n = cfg_.http_session_timeout;
      beast::get_lowest_layer(derived().stream()).expires_after(std::chrono::seconds(n));

      auto self = derived().shared_from_this();
      auto f = [self](auto ec, auto n)
	 { self->on_read(ec, n); };

      parser_.body_limit(cfg_.body_limit);
      http::async_read(derived().stream(), buffer_, parser_, f);
   }
};

class plain_session
    : public session<plain_session>
    , public std::enable_shared_from_this<plain_session>
{
    beast::tcp_stream stream_;

public:
    plain_session(
       tcp::socket&& socket,
       config const& cfg,
       beast::flat_buffer buffer)
    : session<plain_session>(cfg, std::move(buffer))
    , stream_(std::move(socket))
    { }

    beast::tcp_stream& stream() { return stream_; }

    void run()
    {
       do_read();
    }

    void do_eof()
    {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    auto is_ssl() const noexcept { return false; }
};

class ssl_session
    : public session<ssl_session>
    , public std::enable_shared_from_this<ssl_session>
{
    beast::ssl_stream<beast::tcp_stream> stream_;

public:
    // Create the session
    ssl_session(
       tcp::socket&& peer,
       ssl::context& ctx,
       config const& cfg,
       beast::flat_buffer buffer)
       : session<ssl_session>(cfg, std::move(buffer))
       , stream_(std::move(peer), ctx)
    { }

    auto& stream() { return stream_; }

    void run()
    {
        auto self = shared_from_this();
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session.
        net::dispatch(stream_.get_executor(), [self]() {
            beast::get_lowest_layer(
	       self->stream_).expires_after(std::chrono::seconds(30));

            // Perform the SSL handshake
            // Note, this is the buffered version of the handshake.
            self->stream_.async_handshake(
                ssl::stream_base::server,
                self->buffer_.data(),
                beast::bind_front_handler(&ssl_session::on_handshake, self));
        });
    }

    void on_handshake(beast::error_code ec, std::size_t bytes_used)
    {
        if (ec) {
	    log::write(log::level::debug,
		       "on_handshakei (ssl): {0}",
		       ec.message());
            return;
	}

        buffer_.consume(bytes_used);
        do_read();
    }

    void do_eof()
    {
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        stream_.async_shutdown(
            beast::bind_front_handler(
                &ssl_session::on_shutdown,
                shared_from_this()));
    }

    void on_shutdown(beast::error_code ec)
    {
        if (ec) {
	    log::write(log::level::debug,
		       "on_shutdown (ssl): {0}",
		       ec.message());
	}
    }

    auto is_ssl() const noexcept { return true; }
};

}
