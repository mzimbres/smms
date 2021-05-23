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

#include <string>
#include <random>

#include "net.hpp"
#include "utils.hpp"

namespace smms
{

void init_libsodium();
std::string make_hex_digest(std::string const& input);

std::string
make_hex_digest( std::string const& input
               , std::string const& key);

class pwd_gen {
private:
   std::mt19937 gen;
   std::uniform_int_distribution<int> dist;

public:
   // sep below is a character that is not part of the character set
   // used to generate the random strings and can be used as a
   // separator.
   static constexpr char sep = '-';

   pwd_gen();
   std::string operator()(int size);
};

// This function tests if the target the digest has been indeed
// generated using the filename and the secret key shared only by
// smms db and the smms mms.
bool is_valid(pathinfo_type const& info, std::string const& key);

} // smms
