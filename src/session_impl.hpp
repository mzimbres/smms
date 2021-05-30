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

#include <vector>
#include <string>

#include "net.hpp"

namespace smms {

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
   get_cache_control(beast::string_view path) const noexcept;
};

http::response<http::string_body>
make_post_response(
   beast::string_view raw_target,
   http::request_parser<http::string_body> const& parser,
   config const& cfg);

http::response<http::string_body>
make_get_response(
   beast::string_view raw_target,
   http::request_parser<http::string_body> const& parser,
   config const& cfg);

http::response<http::string_body>
make_response(
   http::request_parser<http::string_body> const& parser,
   config const& cfg,
   bool is_ssl);

} // smms
