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

#include "utils.hpp"

#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace smms
{

void create_dir(const char *dir)
{
   char tmp[256];
   char *p = NULL;
   size_t len;

   snprintf(tmp, sizeof(tmp), "%s", dir);
   len = strlen(tmp);

   if (tmp[len - 1] == '/')
      tmp[len - 1] = 0;

   for (p = tmp + 1; *p; p++)
      if (*p == '/') {
         *p = 0;
         mkdir(tmp, 0777);
         *p = '/';
      }

   mkdir(tmp, 0777);
}

std::vector<std::string>
parse_query(std::string const& in)
{
   static char seps[2] = {'=', '&'};

   std::vector<std::string> ret;

   auto const size = std::ssize(in);
   auto a = 0;
   auto b = 1;
   auto last = 0;
   for (auto i = 0; i < size; ++i) {
      if (in[i] == seps[b])
	 return {};

      if (in[i] == seps[a]) {
	 std::swap(a, b);
	 ret.push_back(in.substr(last, i - last));
	 last = i + 1;
      }
   }

   if (last < std::ssize(in))
      ret.push_back(in.substr(last));

   if ((std::size(ret) & 1) != 0)
      return {};

   return ret;
}

string_view make_extension(string_view path)
{
  auto const pos = path.rfind(".");
  if (pos == string_view::npos)
     return string_view{};

  return path.substr(pos);
}

std::pair<string_view, string_view>
split_from_query(string_view path)
{
  auto const pos = path.rfind("?");
  if (pos == string_view::npos) {
     // The string has no query.
     return {path, {}};
  }

  auto const first = path.substr(0, pos);
  if (pos + 1 == std::size(path)) {
     // The string has a ? but no query.
     return {first, {}};
  }

  auto const second = path.substr(pos + 1);
  return {first, second};
}

std::string parse_dir(std::string const& target)
{
   auto const pos = target.rfind('/');
   if (pos == std::string::npos)
      return {};

   return target.substr(0, pos);
}

std::string
get_field_value(
   std::vector<std::string> const& queries,
   std::string const& field)
{
   auto match =
      std::find(
	 std::cbegin(queries),
	 std::cend(queries),
	 field);

   auto const end = std::cend(queries);
   if (match != end && match != std::prev(end))
      return *++match;

   return {};
}

int stoi_nothrow(std::string const& s, error_code& ec)
{
   try {
      return std::stoi(s);
   } catch (...) {
      ec = error_code::invalid;
      return {};
   }
}

} // smms
