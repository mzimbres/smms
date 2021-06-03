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

namespace
{

constexpr char hextable[] = "0123456789abcdef";
constexpr char pwdchars[] = "abcdefghijklmnopqrstuvwxyz0123456789";

//constexpr auto hash_size = crypto_generichash_BYTES;
constexpr auto hash_size = crypto_generichash_BYTES_MIN;
using hash_type = std::array<unsigned char, hash_size>;

char low_to_char(unsigned char a)
{
    return hextable[a & 0x0f];
}

char high_to_char(unsigned char a)
{
    return hextable[(a & 0xf0) >> 4];
}

std::string hash_to_string(hash_type const& hash)
{
  std::string output;
  output.reserve(2 * std::size(hash));
  for (auto i = 0; i < std::ssize(hash); ++i) {
    output.push_back(high_to_char(hash[i]));
    output.push_back(low_to_char(hash[i]));
  }

  return output;
}

}

namespace smms
{

pwd_gen::pwd_gen()
: gen {std::random_device{}()}
, dist {0, sizeof pwdchars - 2}
{}

std::string pwd_gen::operator()(int pwd_size)
{
   std::string pwd;
   for (auto i = 0; i < pwd_size; ++i) {
      pwd.push_back(pwdchars[dist(gen)]);
   }

   return pwd;
}

std::string make_hex_digest(std::string const& input)
{
   auto const* p1 = reinterpret_cast<unsigned char const*>(input.data());

   hash_type hash;
   crypto_generichash(hash.data(), std::size(hash),
      p1, std::size(input), nullptr, 0);

   return hash_to_string(hash);
}

std::string
make_hex_digest( std::string const& input
               , std::string const& key)
{
   if (std::size(key) != crypto_generichash_KEYBYTES)
      return {};

   auto const* p1 = reinterpret_cast<unsigned char const*>(input.data());
   auto const* p2 = reinterpret_cast<unsigned char const*>(key.data());

   hash_type hash;
   crypto_generichash( hash.data(), std::size(hash)
                     , p1, std::size(input)
                     , p2, std::size(key));

   return hash_to_string(hash);
}

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
   pwd_gen gen;
   auto const tmp = gen(std::size(key));
   std::copy(std::cbegin(tmp), std::cend(tmp), std::begin(key));
   return key;
}

} // hmacsha256

} // smms
