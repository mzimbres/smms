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
#include "logger.hpp"
#include "session.hpp"

#include <sys/types.h>
#include <sys/socket.h>

namespace smms
{

class acceptor {
private:
   ssl::context& ctx_;
   net::ip::tcp::acceptor acceptor_;

   void do_accept(config const& w);
   void on_accept(config const& w,
                  boost::system::error_code ec,
                  net::ip::tcp::socket peer);

public:
   acceptor(net::io_context& ioc, ssl::context& ctx);
   void run(config const& w,
            unsigned short port,
            int max_listen_connections);

   void shutdown();
};

}

