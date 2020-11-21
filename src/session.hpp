/* Copyright (c) 2018-2020 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This file is part of tsvtree.
 *
 * tsvtree is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tsvtree is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tsvtree.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "net.hpp"

#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>

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
private:
   using req_body_parser_type = http::request_parser<http::file_body>;
   using resp_body_type = http::response<http::file_body>;

protected:
   beast::flat_buffer buffer_ {8192};

private:
   // Read header firstknow in advance the file name.
   http::request_parser<http::empty_body> header_parser_;

   // std::unique_ptr here required as beast does not offer
   // correct move semantics for req_body_parser_type.
   std::unique_ptr<req_body_parser_type> body_parser_;
   http::response<http::dynamic_body> resp_;
   std::unique_ptr<resp_body_type> file_body_resp_;
   
   config const& cfg_;

    Derived& derived()
       { return static_cast<Derived&>(*this); }

   void post_handler(beast::string_view target)
   {
      // Before posting we check if the digest and the rest of the target
      // have been produced by the same key.
      std::string path;
      auto const pinfo = make_path_info(target);
      if (is_valid(pinfo, cfg_.key)) {
	 path = cfg_.doc_root;
	 path.append(target.data(), std::size(target));
	 log::write(log::level::debug , "Post dir: {0}", path);
	 auto full_dir = cfg_.doc_root + "/";
	 full_dir.append(pinfo[0].data(), std::size(pinfo[0]));
	 log::write(log::level::debug , "MMS dir: {0}", full_dir);
	 create_dir(full_dir.data());
      }

      log::write(log::level::debug , "Post full dir: {0}", path);

      if (std::empty(path)) {
	 resp_.result(http::status::bad_request);
	 resp_.set(http::field::content_type, "text/plain");
	 beast::ostream(resp_.body()) << "Invalid signature.\r\n";
         resp_.set(http::field::content_length,
                   beast::to_static_string(std::size(resp_.body())));
	 write_response();
	 return;
      }

      // The filename contains a valid signature, we can open the file
      // and read the body to save the image.

      body_parser_ = std::make_unique< req_body_parser_type
				    >(std::move(header_parser_));

      beast::error_code ec;
      body_parser_->get()
	 .body()
	 .open(path.data(),
	       beast::file_mode::write_new,
	       ec);

      if (ec) {
         log::write(log::level::info , "post_handler: {0}", ec.message());
         resp_.result(http::status::bad_request);
         resp_.set(http::field::content_type, "text/plain");
         beast::ostream(resp_.body()) << "Error\r\n";
         resp_.set(http::field::content_length,
                   beast::to_static_string(std::size(resp_.body())));
         write_response();
       return;
    }

    auto self = derived().shared_from_this();
    auto f = [self](auto ec, auto n)
       { self->on_read_post_body(ec, n); };

    http::async_read(derived().stream(), buffer_, *body_parser_, f);
 }

   void get_handler(beast::string_view raw_target)
   {
      log::write(log::level::debug, "Get target: {0}", raw_target);

      auto const target = remove_queries(raw_target);
      assert(!std::empty(target));
      auto path = cfg_.doc_root;
      path.append(target.data(), std::size(target));
      if (std::size(target) == 1)
	 path += cfg_.default_file;

      auto final_path = path;
      auto gzip = false;
      if (has_mime(path, cfg_.gzip_mimes)) {
        // We must to server this mime type with pre-compressed files. That
        // means we have to add a ".gz" to the path but only if the client
        // accepts that encoding. If it doesn't we do not change the filename.
        auto const match = header_parser_.get().find(http::field::accept_encoding);
        if (match != std::end(header_parser_.get())) {
	   if (match->value().find("gzip") != std::string::npos) {
             // check whether gzip version exists.
             if (fs::exists(fs::path(final_path + ".gz"))) {
               final_path += ".gz";
               gzip = true;
             }
           }
        }
      }

      log::write(log::level::debug, "Get target path: {0}", final_path);

      beast::error_code ec;
      http::file_body::value_type body;
      body.open(final_path.data(), beast::file_mode::scan, ec);

      if (ec) {
	 log::write(log::level::debug, "get_handler: {0}", ec.message());
	 resp_.result(http::status::not_found);
	 resp_.set(http::field::content_type, mime_type(".txt"));
	 beast::ostream(resp_.body()) << "File not found.\r\n";
         resp_.set(http::field::content_length,
                   beast::to_static_string(std::size(resp_.body())));
	 write_response();
	 return;
      }

      file_body_resp_ = std::make_unique<resp_body_type>(
	 std::piecewise_construct,
	 std::make_tuple(std::move(body)),
	 std::make_tuple(http::status::ok, header_parser_.get().version()));

      file_body_resp_->set(http::field::server, cfg_.server_name);
      file_body_resp_->set(http::field::content_type, mime_type(path));
      file_body_resp_->content_length(std::size(body));
      file_body_resp_->keep_alive(header_parser_.keep_alive());
      file_body_resp_->set(http::field::access_control_allow_origin,
			   cfg_.allow_origin);
      if (gzip)
        file_body_resp_->set(http::field::content_encoding, "gzip");

      if (cfg_.set_cache_control())
        file_body_resp_->set(
           http::field::cache_control,
           cfg_.get_cache_control(path));

      auto self = derived().shared_from_this();
      auto f = [self](auto ec, auto)
	 { self->derived().do_eof(); };

      http::async_write(derived().stream(), *file_body_resp_, f);
   }

   void on_read_post_body(boost::system::error_code ec, std::size_t n)
   {
      log::write(log::level::debug, "Body size: {0}.", n);

      if (ec) {
	 // TODO: Use the correct code.
	 resp_.result(http::status::bad_request);
	 resp_.set(http::field::content_type, "text/plain");
	 beast::ostream(resp_.body()) << "File not found\r\n";
	 log::write( log::level::info
		   , "on_read_post_body: {0}."
		   , ec.message());
      } else {
	 resp_.result(http::status::ok);
	 resp_.set(http::field::server, cfg_.server_name);
	 resp_.set(http::field::access_control_allow_origin,
		   cfg_.allow_origin);
      }

      resp_.set(http::field::content_length,
                beast::to_static_string(std::size(resp_.body())));
      write_response();
   }

   void set_redirect(beast::string_view target)
   {
      log::write(log::level::debug,
		 "Redirecting to {0}",
		 cfg_.redirect_url);

      resp_.result(http::status::moved_permanently);
      std::string url = cfg_.redirect_url;
      url.append(target.data(), std::size(target));
      resp_.set(http::field::location, url);
      resp_.set(http::field::content_length, beast::to_static_string(0));
      resp_.set(http::field::access_control_allow_origin,
			   cfg_.allow_origin);
   }

   void on_read_header(boost::system::error_code ec, std::size_t n)
   {
      if (!log::ignore(log::level::debug)) { // Optimization.
	 for (auto const& field : header_parser_.get())
	    log::write(log::level::debug, "   {0}: {1}", field.name(), field.value());
      }

      auto const target = header_parser_.get().target();
      auto const match = header_parser_.get().find(http::field::host);
      auto no_host_match = false;
      if (match != std::end(header_parser_.get())) {
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

      resp_.version(header_parser_.get().version());
      resp_.keep_alive(false);

      switch (header_parser_.get().method()) {
      case http::verb::post: post_handler(target); break;
      case http::verb::get: get_handler(target); break;
      default:
      {
	 resp_.result(http::status::bad_request);
	 resp_.set(http::field::content_type, "text/plain");
         resp_.set(http::field::content_length,
                   beast::to_static_string(std::size(resp_.body())));
	 write_response();
      }
      break;
      }
   }

   void write_response()
   {
      auto self = derived().shared_from_this();
      auto f = [self](auto ec, auto)
	 { self->derived().do_eof(); };

      http::async_write(derived().stream(), resp_, f);
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
	 { self->on_read_header(ec, n); };

      header_parser_.body_limit(cfg_.body_limit);
      http::async_read_header(derived().stream(), buffer_, header_parser_, f);
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
