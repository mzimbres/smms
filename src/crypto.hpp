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

#include <string>
#include <random>

#include "net.hpp"

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

using path_info = std::array<beast::string_view, 4>;

// Expects a target in the form
//
//    /x/x/xx/filename:digest.jpg
//
// where the extension is optional and returns an array in the form
//
// a[0] = x/x/xx
// a[1] = filename
// a[2] = digest
// a[3] = jpg
path_info make_path_info(beast::string_view target);

// This function tests if the target the digest has been indeed
// generated using the filename and the secret key shared only by
// smms db and the smms mms.
bool is_valid(path_info const& info, std::string const& key);

/* Parses a string in the form 
 *
 *   /hash/dir1/dir2/dir3/file.ext
 *
 * and returns a pair consisting of
 *
 *   1. hash
 *   2. /dir1/dir2/dir3/file.ext
 */
std::pair<std::string, std::string>
parse_hash(std::string const& target);

/* Parses a string in the form
 *
 *   /dir1/dir2/dir3/file.ext
 *
 * and returns
 *    
 *   /prefix/dir1/dir2/dir3/
 */
std::string read_dir(std::string const& s, std::string prefix);

}

