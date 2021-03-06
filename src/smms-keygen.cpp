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

#include <string>
#include <iostream>

#include <sodium.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "crypto.hpp"

namespace po = boost::program_options;

namespace smms
{

struct config {
   bool help = false;
   bool make_key {false};
   std::string make_hmac;
   std::string make_http_target;
   std::string key;
};

auto make_cfg(int argc, char* argv[])
{
   config cfg;

   po::options_description desc("Options");
   desc.add_options()
   ("help,h", "Help description")
   ("make-key,k", "Output a randomly generated key.")
   ("make-hmac,d", po::value<std::string>(&cfg.make_hmac), "Generates the hmacsha256 for the given input.")
   ("key,e", po::value<std::string>(&cfg.key), "The key that should be used to generate the hmac.")
   ;

   po::positional_options_description pos;
   pos.add("config", -1);

   po::variables_map vm;        
   po::store(po::command_line_parser(argc, argv).
      options(desc).positional(pos).run(), vm);
   po::notify(vm);    

   if (vm.count("help")) {
      std::cout << desc << "\n";
      return config {true};
   }

   if (vm.count("make-key")) {
      return config {false, true};
   }

   return cfg;
}

}

using namespace smms;

int main(int argc, char* argv[])
{
   try {
      auto const cfg = make_cfg(argc, argv);
      if (cfg.help)
         return 0;

      init_libsodium();

      if (cfg.make_key) {
	 auto const hex_key = hmacsha256::make_random_hex_key();
         std::cout << hex_key << std::endl;
         return 0;
      }

      if (!std::empty(cfg.make_hmac)) {
         if (std::empty(cfg.key)) {
            std::cerr << "Error: No key provided." << std::endl;
	    return 1;
	 }

	 hmacsha256::key_type key;

	 auto const ret =
	    sodium_hex2bin(
	       key.data(),
	       key.size(),
	       cfg.key.data(),
	       cfg.key.size(),
	       nullptr,
	       nullptr,
	       nullptr);

	 if (ret == -1) {
	    std::cerr << "Error: invalid key." << std::endl;
	    return -1;
	 }

	 auto const auth = hmacsha256::make_auth(cfg.make_hmac, key);
	 std::string out(2 * std::size(auth) + 1, 0);

	 sodium_bin2hex(
	    out.data(),
	    std::size(out),
	    auth.data(),
	    std::size(auth));

	 std::cout << out << std::endl;
         return 0;
      }

      std::cerr << "No input specified." << std::endl;

   } catch(std::exception const& e) {
      std::cerr << e.what() << std::endl;
      return 1;
   }
}
