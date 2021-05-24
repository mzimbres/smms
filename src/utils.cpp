#include "utils.hpp"

#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace smms
{

void create_dir(const char *dir)
{
   char tmp[256];
   char *p = NULL;
   size_t len;

   snprintf(tmp, sizeof(tmp), "%s", dir);
   len = strlen(tmp);

   if (tmp[len - 1] == '/')
      tmp[len - 1] = 0;

   for (p = tmp + 1; *p; p++)
      if (*p == '/') {
         *p = 0;
         mkdir(tmp, 0777);
         *p = '/';
      }

   mkdir(tmp, 0777);
}

std::vector<std::string>
parse_query(std::string const& in)
{
   static char seps[2] = {'=', '&'};

   std::vector<std::string> ret;

   auto const size = std::ssize(in);
   auto a = 0;
   auto b = 1;
   auto last = 0;
   for (auto i = 0; i < size; ++i) {
      if (in[i] == seps[b])
	 return {};

      if (in[i] == seps[a]) {
	 std::swap(a, b);
	 ret.push_back(in.substr(last, i - last));
	 last = i + 1;
      }
   }

   if (last < std::ssize(in))
      ret.push_back(in.substr(last));

   if ((std::size(ret) & 1) != 0)
      return {};

   return ret;
}

beast::string_view make_extension(beast::string_view path)
{
  auto const pos = path.rfind(".");
  if (pos == beast::string_view::npos)
     return beast::string_view{};

  return path.substr(pos);
}

std::pair<beast::string_view, beast::string_view>
split_from_query(beast::string_view path)
{
  auto const pos = path.rfind("?");
  if (pos == beast::string_view::npos) {
     // The string has no query.
     return {path, {}};
  }

  auto const first = path.substr(0, pos);
  if (pos + 1 == std::size(path)) {
     // The string has a ? but no query.
     return {first, {}};
  }

  auto const second = path.substr(pos + 1);
  return {first, second};
}


pathinfo_type make_path_info(beast::string_view target)
{
   pathinfo_type pinfo;

   std::array<char, 4> delimiters {{'/', '/', '-', '.'}};
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

std::string parse_dir(std::string const& target)
{
   auto const pos = target.rfind('/');
   if (pos == std::string::npos)
      return {};

   return target.substr(0, pos);
}

} // smms
