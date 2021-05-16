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

#include "acceptor.hpp"

namespace smms
{

class detect_session : public std::enable_shared_from_this<detect_session> {
private:
   beast::tcp_stream stream_;
   ssl::context& ctx_;
   beast::flat_buffer buffer_;
   config const& cfg_;

public:
   detect_session(tcp::socket&& socket, ssl::context& ctx, config const& w)
   : stream_(std::move(socket))
   , ctx_(ctx)
   , cfg_ {w}
   { }

   void run()
   {
      beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

      async_detect_ssl(
         stream_,
         buffer_,
         beast::bind_front_handler(&detect_session::on_detect, shared_from_this()));
   }

   void on_detect(beast::error_code ec, bool result)
   {
      if (ec) {
	 log::write(log::level::debug , "on_detect: {0}", ec.message());
         return;
      }

      if (result) {
          std::make_shared<ssl_session>(
              stream_.release_socket(),
              ctx_,
              cfg_,
              std::move(buffer_))->run();
          return;
      }

      std::make_shared<plain_session>(
         stream_.release_socket(),
	 cfg_,
         std::move(buffer_))->run();
   }
};

acceptor::acceptor(net::io_context& ioc, ssl::context& ctx)
: ctx_(ctx)
, acceptor_ {ioc}
{ }

void acceptor::do_accept(config const& w)
{
   auto f = [this, &w](auto const& ec, auto socket)
      { on_accept(w, ec, std::move(socket)); };

   acceptor_.async_accept(f);
}

void acceptor::
on_accept(config const& w,
          boost::system::error_code ec,
          net::ip::tcp::socket peer)
{
   if (ec) {
      if (ec == net::error::operation_aborted) {
         log::write(log::level::info, "Stopping accepting connections");
         return;
      }

      log::write(log::level::info, "listener::on_accept: {0}", ec.message());
   } else {
      std::make_shared<detect_session>(
	  std::move(peer),
	  ctx_,
	  w)->run();
   }

   do_accept(w);
}

void acceptor::
run(config const& w,
    unsigned short port,
    int max_listen_connections)
{
   tcp::endpoint endpoint {tcp::v4(), port};
   acceptor_.open(endpoint.protocol());

   int one = 1;
   auto const ret =
      setsockopt( acceptor_.native_handle()
                , SOL_SOCKET
                , SO_REUSEPORT
                , &one, sizeof(one));

   if (ret == -1) {
      log::write( log::level::err
                , "Unable to set socket option SO_REUSEPORT: {0}"
                , strerror(errno));
   }

   acceptor_.bind(endpoint);

   boost::system::error_code ec;
   acceptor_.listen(max_listen_connections, ec);

   if (ec) {
      log::write(log::level::info, "acceptor::run: {0}.", ec.message());
   } else {
      log::write(log::level::info,
                 "acceptor:run: Listening on {}",
                 acceptor_.local_endpoint());

      log::write(log::level::info,
                 "acceptor:run: TCP backlog set to {}",
                 max_listen_connections);

      do_accept(w);
   }
}

void acceptor::shutdown()
{
   if (acceptor_.is_open()) {
      boost::system::error_code ec;
      acceptor_.cancel(ec);
      if (ec) {
         log::write(log::level::info,
                    "acceptor::shutdown: {0}.",
                    ec.message());
      }
   }
}

}
