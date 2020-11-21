#include "utils.hpp"

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

}

