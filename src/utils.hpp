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

namespace smms
{

using pathinfo_type = std::array<beast::string_view, 4>;

void create_dir(const char *dir);

std::vector<std::string>
parse_query(std::string const& in);

beast::string_view make_extension(beast::string_view path);

// Splits the url into a target and query. For example
//
//    http://foo.bar/a/b/index.html?one=two
//
// Returns
//
// First: http://foo.bar/a/b/index.html
// Second: one=two
std::pair<beast::string_view, beast::string_view>
split_from_query(beast::string_view path);

/* Parses a http target in the form
 *
 *   /dir1/dir2/dir3/.../file.ext
 *
 * and returns the directory part
 *    
 *   /prefix/dir1/dir2/dir3/
 *
 * NOTE: The target is assumed to contain no queries.
 */
std::string parse_dir(std::string const& target);

std::string
get_field_value(
   std::vector<std::string> const& queries,
   std::string const& field);

enum class error_code
{ ok = 0
, invalid
};

int stoi_nothrow(std::string const& s, error_code& ec);

} // smms
