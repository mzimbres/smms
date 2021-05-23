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
   std::string make_digest;
   std::string make_http_target;
   std::string key;
};

auto make_cfg(int argc, char* argv[])
{
   config cfg;

   po::options_description desc("Options");
   desc.add_options()
   ("help,h", "Produces help description")
   ("make-key,k", "Output a randomly generated key to sign urls.")
   ("make-digest,d", po::value<std::string>(&cfg.make_digest), "Generates the digest for a given http target.")
   ("make-http-target,t", po::value<std::string>(&cfg.make_http_target), "Generates the http target that can be used to post on the server.")
   ("key,e", po::value<std::string>(&cfg.key), "The key that should be used to generate the digest.")
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

      if (cfg.make_key) {
         pwd_gen gen;
         std::cout << gen(crypto_generichash_KEYBYTES) << std::endl;
         return 0;
      }

      if (!std::empty(cfg.make_digest)) {
         if (std::empty(cfg.key))
            std::cout << make_hex_digest(cfg.make_digest) << std::endl;
         else
            std::cout << make_hex_digest(cfg.make_digest, cfg.key) << std::endl;
         return 0;
      }

      if (!std::empty(cfg.make_http_target)) {
         std::string str = cfg.make_http_target;
         if (std::empty(cfg.key)) {
            str += "?hmac=";
            str += make_hex_digest(cfg.make_http_target);
	 } else {
            str += "?hmac=";
            str += make_hex_digest(cfg.make_http_target, cfg.key);
	 }

         std::cout << str << std::endl;
         return 0;
      }

   } catch(std::exception const& e) {
      std::cerr << e.what() << std::endl;
      return 1;
   }
}

