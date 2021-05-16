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

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include "net.hpp"
#include "crypto.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "acceptor.hpp"

namespace smms {

struct server_cfg {
   bool exit = false;
   unsigned short http_port;
   unsigned short https_port;
   log::level logfilter;
   config session_cfg;
   int max_listen_connections;

   std::string ssl_cert_file;
   std::string ssl_priv_key_file;
   std::string ssl_dh_file;

   auto with_ssl() const noexcept
   {
      auto const r = std::empty(ssl_cert_file) ||
                     std::empty(ssl_priv_key_file) ||
                     std::empty(ssl_dh_file);
      return !r;
   }

   auto has_compatible_cache_control() const noexcept
   {
     return
       std::size(session_cfg.local_cache_control_mime) ==
       std::size(session_cfg.local_cache_control_value);
   }
};

namespace po = boost::program_options;

auto make_cfg(int argc, char* argv[])
{
   server_cfg cfg;
   std::string conf_file;
   std::string logfilter_str;

   po::options_description desc("Options");
   desc.add_options()
   ("help,h", "Produces help message")
   ("host-name", po::value<std::vector<std::string>>(&cfg.session_cfg.host_names))
   ("gzip-mimes", po::value<std::vector<std::string>>(&cfg.session_cfg.gzip_mimes))
   ("local-cache-control-mime", po::value<std::vector<std::string>>(&cfg.session_cfg.local_cache_control_mime))
   ("local-cache-control-value", po::value<std::vector<std::string>>(&cfg.session_cfg.local_cache_control_value))
   ("server-name", po::value<std::string>(&cfg.session_cfg.server_name))
   ("redirect-url", po::value<std::string>(&cfg.session_cfg.redirect_url))
   ("doc-root", po::value<std::string>(&cfg.session_cfg.doc_root)->default_value("/data/www"))
   ("body-limit", po::value<std::uint64_t>(&cfg.session_cfg.body_limit)->default_value(1000000))
   ("config", po::value<std::string>(&conf_file))
   ("default-file", po::value<std::string>(&cfg.session_cfg.default_file))
   ("default-cache-control", po::value<std::string>(&cfg.session_cfg.default_cache_control))
   ("http-port", po::value<unsigned short>(&cfg.http_port)->default_value(80))
   ("https-port", po::value<unsigned short>(&cfg.https_port)->default_value(443))
   ("log-level", po::value<std::string>(&logfilter_str)->default_value("debug"))
   ("key", po::value<std::string>(&cfg.session_cfg.key))
   ("allow-origin", po::value<std::string>(&cfg.session_cfg.allow_origin)->default_value("*"))
   ("max-listen-connections", po::value<int>(&cfg.max_listen_connections)->default_value(511))
   ("ssl-certificate-file", po::value<std::string>(&cfg.ssl_cert_file))
   ("ssl-private-key-file", po::value<std::string>(&cfg.ssl_priv_key_file))
   ("ssl-dh-file", po::value<std::string>(&cfg.ssl_dh_file))
   ;

   po::positional_options_description pos;
   pos.add("config", -1);

   po::variables_map vm;        
   po::store(po::command_line_parser(argc, argv).
         options(desc).positional(pos).run(), vm);
   po::notify(vm);    

   if (!std::empty(conf_file)) {
      std::ifstream ifs {conf_file};
      if (ifs) {
         po::store(po::parse_config_file(ifs, desc, true), vm);
         notify(vm);
      }
   }

   if (!cfg.has_compatible_cache_control()) {
      log::write(log::level::err, "Incompatible size of local-cache-control-* fields.");
      return server_cfg {true};
   }

   if (vm.count("help")) {
      std::cout << desc << "\n";
      return server_cfg {true};
   }

   cfg.logfilter = log::to_level<log::level>(logfilter_str);
   return cfg;
}

}

using namespace smms;

int main(int argc, char* argv[])
{
   try {
      auto const cfg = make_cfg(argc, argv);
      if (cfg.exit)
         return 0;

      init_libsodium();
      log::upto(cfg.logfilter);

      if (cfg.http_port == 0 && cfg.https_port == 0) {
	 log::write(log::level::notice,
	            "No ports have been enabled, leaving ...");
	 return 0;
      }

      net::io_context ioc {BOOST_ASIO_CONCURRENCY_HINT_UNSAFE};
      ssl::context ctx {ssl::context::tlsv12};
      config session_cfg {cfg.session_cfg};

      std::unique_ptr<acceptor> http;
      if (cfg.http_port != 0) {
	 http = std::make_unique<acceptor>(ioc, ctx);
	 http->run(session_cfg, cfg.http_port, cfg.max_listen_connections);
      }

      std::unique_ptr<acceptor> https;
      if (cfg.https_port != 0 && cfg.with_ssl()) {
         auto const b =
            load_ssl( ctx
                    , cfg.ssl_cert_file
                    , cfg.ssl_priv_key_file
                    , cfg.ssl_dh_file);
         if (!b) {
            log::write(log::level::notice, "Unable to load ssl files.");
         } else {
            log::write(log::level::notice, "Load ssl files.");
	    https = std::make_unique<acceptor>(ioc, ctx);
	    https->run(session_cfg, cfg.https_port, cfg.max_listen_connections);
	 }
      }

      ioc.run();
   } catch(std::exception const& e) {
      log::write(log::level::notice, e.what());
      log::write(log::level::notice, "Exiting with status 1 ...");
      return 1;
   }
}

