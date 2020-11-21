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

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace net = boost::asio;
namespace ip = net::ip;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;

namespace smms
{

bool load_ssl( ssl::context& ctx
             , std::string const& ssl_cert_file
             , std::string const& ssl_priv_key_file
             , std::string const& ssl_dh_file);

beast::string_view make_extension(beast::string_view path);
beast::string_view remove_queries(beast::string_view path);
beast::string_view mime_type(beast::string_view path);
bool has_mime(beast::string_view path, std::vector<std::string> const& mimes);

}

