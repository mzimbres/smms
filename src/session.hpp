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

#include <vector>
#include <iterator>

#include "logger.hpp"
#include "session_impl.hpp"

namespace smms
{

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
      response_ = make_post_response(raw_target, parser_, cfg_);
      write_response();
   }

   void get_handler(beast::string_view raw_target)
   {
      response_ = make_get_response(raw_target, parser_, cfg_);
      write_response();
   }

   void on_read(boost::system::error_code ec, std::size_t n)
   {
      log::write(
	 log::level::debug,
	 "on_read: number of bytes read {0}.",
	 n);

      if (ec) {
	 response_.result(http::status::bad_request);
	 response_.set(http::field::content_type, "text/plain");
	 response_.body() = "Invalid body size.\r\n";
	 response_.set(http::field::content_length,
		       beast::to_static_string(std::size(response_.body())));
      } else {
	 auto const is_ssl = derived().is_ssl();
	 response_ = make_response(parser_, cfg_, is_ssl);
      }

      write_response();
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
