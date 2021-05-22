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

#include "net.hpp"

#include <algorithm>

#include "logger.hpp"

namespace smms
{

bool load_ssl( ssl::context& ctx
             , std::string const& ssl_cert_file
             , std::string const& ssl_priv_key_file
             , std::string const& ssl_dh_file)
{
   boost::system::error_code ec;

   // At the moment we do not have certificate with password.
   ctx.set_password_callback( [](auto n, auto k) { return ""; }
                            , ec);

   if (ec) {
      log::write(log::level::emerg, "{}", ec.message());
      return false;
   }

   ec = {};

   ctx.set_options(
      ssl::context::default_workarounds |
      ssl::context::no_sslv2 |
      ssl::context::single_dh_use, ec);

   if (ec) {
      log::write( log::level::emerg
                , "load_ssl: set_options: {}"
                , ec.message());
      return false;
   }

   ec = {};

   ctx.use_certificate_chain_file(ssl_cert_file, ec);

   if (ec) {
      log::write( log::level::emerg
                , "load_ssl use_certificate_chain_file: {}"
                , ec.message());
      return false;
   }

   ec = {};

   ctx.use_private_key_file( ssl_priv_key_file
                           , ssl::context::file_format::pem);

   if (ec) {
      log::write( log::level::emerg
                , "load_ssl use_private_key_file: {}"
                , ec.message());
      return false;
   }

   ec = {};

   ctx.use_tmp_dh_file(ssl_dh_file, ec);

   if (ec) {
      log::write( log::level::emerg
                , "load_ssl use_tmp_dh_file: {}"
                , ec.message());
      return false;
   }

   return true;
}

beast::string_view remove_queries(beast::string_view path)
{
  auto const pos = path.rfind("?");
  if (pos == beast::string_view::npos)
     return path;

  return path.substr(0, pos);
}

beast::string_view make_extension(beast::string_view path)
{
  auto const pos = path.rfind(".");
  if (pos == beast::string_view::npos)
     return beast::string_view{};

  return path.substr(pos);
}

beast::string_view mime_type(beast::string_view path)
{
   using beast::iequals;

   auto const ext = make_extension(path);

   if (iequals(ext, ".htm"))  return "text/html";
   if (iequals(ext, ".html")) return "text/html";
   if (iequals(ext, ".php"))  return "text/html";
   if (iequals(ext, ".css"))  return "text/css";
   if (iequals(ext, ".txt"))  return "text/plain";
   if (iequals(ext, ".js"))   return "application/javascript";
   if (iequals(ext, ".json")) return "application/json";
   if (iequals(ext, ".xml"))  return "application/xml";
   if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
   if (iequals(ext, ".flv"))  return "video/x-flv";
   if (iequals(ext, ".png"))  return "image/png";
   if (iequals(ext, ".jpe"))  return "image/jpeg";
   if (iequals(ext, ".jpeg")) return "image/jpeg";
   if (iequals(ext, ".jpg"))  return "image/jpeg";
   if (iequals(ext, ".gif"))  return "image/gif";
   if (iequals(ext, ".bmp"))  return "image/bmp";
   if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
   if (iequals(ext, ".tiff")) return "image/tiff";
   if (iequals(ext, ".tif"))  return "image/tiff";
   if (iequals(ext, ".svg"))  return "image/svg+xml";
   if (iequals(ext, ".svgz")) return "image/svg+xml";

   return "application/text";
}

bool has_mime(beast::string_view path, std::vector<std::string> const& mimes)
{
    using beast::iequals;

    auto const ext = make_extension(path);

    auto f = [&](auto const& mime)
      { return iequals(ext, mime); };

    return std::any_of(std::cbegin(mimes), std::cend(mimes), f);
}

}

