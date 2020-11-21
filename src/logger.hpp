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

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <syslog.h>

namespace smms { namespace log {

enum class level
{ emerg
, alert
, crit
, err
, warning
, notice
, info
, debug
};

template <class T>
T to_level(std::string const& ll)
{
   if (ll == "emerg")   return T::emerg;
   if (ll == "alert")   return T::alert;
   if (ll == "crit")    return T::crit;
   if (ll == "err")     return T::err;
   if (ll == "warning") return T::warning;
   if (ll == "notice")  return T::notice;
   if (ll == "info")    return T::info;
   if (ll == "debug")   return T::debug;

   return T::debug;
}

void upto(level ll);

namespace global {extern level filter;}

inline                                                                                                                                                      
auto ignore(level ll)                                                                                                                                
{                                                                                                                                                           
   return ll > global::filter;                                                                                                                           
}

template <class... Args>
void write(level ll, char const* fmt, Args const& ... args)
{
   if (ll > global::filter)
      return;

   std::clog << fmt::format(fmt, args...) << std::endl;
}

}
}

