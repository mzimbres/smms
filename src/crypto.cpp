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

#include "crypto.hpp"

#include <array>
#include <string>
#include <algorithm>
#include <stdexcept>

#include <sodium.h>

namespace smms
{

void init_libsodium()
{
   if (sodium_init() == -1)
      throw std::runtime_error("Error: Cannot initialize libsodium.");
}

namespace hmacsha256 {

auth_type
make_auth(std::string const& in, key_type const& key)
{
   auth_type ret;

   crypto_auth_hmacsha256(
      ret.data(),
      reinterpret_cast<unsigned char const*>(in.data()),
      std::size(in),
      key.data());

   return ret;
}

int verify(auth_type const& auth, std::string const& in, key_type const& key)
{
   return
   crypto_auth_hmacsha256_verify(
      auth.data(),
      reinterpret_cast<unsigned char const*>(in.data()),
      std::size(in),
      key.data());
}

key_type make_random_key()
{
   key_type key;
   randombytes_buf(key.data(), std::size(key));
   return key;
}

// Produces a hmac key and converts it to a hex string.
std::string make_random_hex_key()
{
   auto const key = make_random_key();
   std::string ret(2 * std::size(key) + 1, 0);
   sodium_bin2hex(ret.data(), std::size(ret), key.data(), std::size(key));
   return ret;
}

} // hmacsha256
} // smms
