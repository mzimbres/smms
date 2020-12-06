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

#include "crypto.hpp"

#include <sodium.h>

#include <array>
#include <string>
#include <iostream>
#include <sstream>

#include "utils.hpp"

namespace
{

constexpr char hextable[] = "0123456789abcdef";

// Do not change this character set without taking pwd_gen::sep into
// consideration.
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

path_info make_path_info(beast::string_view target)
{
   path_info pinfo;

   std::array<char, 4> delimiters {{'/', '/', pwd_gen::sep, '.'}};
   auto j = std::ssize(delimiters) - 1;
   auto k = std::ssize(target);
   for (auto i = k - 1; i >= 0; --i) {
      if (target[i] == delimiters[j]) {
         pinfo[j] = target.substr(i + 1, k - i - 1);
         k = i;
         --j;
      }

      if (j == 0) {
         pinfo[0] = target.substr(1, k - 1);
         break;
      }
   }

   return pinfo;
}

bool is_valid(path_info const& info, std::string const& key)
{
   std::string path = "/";
   path.append(info[0].data(), std::size(info[0]));
   path += "/";
   path.append(info[1].data(), std::size(info[1]));

   auto const digest = make_hex_digest(path, key);
   auto const digest_size = std::size(info[2]);
   if (digest_size != std::size(digest))
      return false;

   return digest.compare(0, digest_size, info[2].data(), digest_size) == 0;
}

std::pair<std::string, std::string>
parse_hash(std::string const& target)
{
   if (std::empty(target))
      return {};

   if (target.front() != '/')
      return {};

   auto const pos = target.find("/", 1);
   if (pos == std::string::npos)
      return {};

   auto const a = target.substr(1, pos - 1);
   auto const b = target.substr(pos);
   return {a, b};
}

std::string read_dir(std::string const& s, std::string prefix)
{
   auto const pos = s.rfind('/');
   if (pos == std::string::npos)
      return {};

   if (std::empty(prefix))
       return s.substr(0, pos);

   prefix += s.substr(0, pos);
   return prefix;

}

}

