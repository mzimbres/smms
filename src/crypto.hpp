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

#include <array>
#include <string>

#include <sodium.h>

namespace smms
{

void init_libsodium();
std::string make_hex_digest(std::string const& input);

std::string
make_hex_digest( std::string const& input
               , std::string const& key);

namespace hmacsha256 {

using key_type =
   std::array<unsigned char, crypto_auth_hmacsha256_KEYBYTES>;

using auth_type =
   std::array<unsigned char, crypto_auth_hmacsha256_BYTES>;

auth_type
make_auth(
   std::string const& in,
   key_type const& key);

int
verify(
   auth_type const& auth,
   std::string const& in,
   key_type const& key);

key_type make_random_key();
std::string make_random_hex_key();

} // hmacsha256

} // smms
