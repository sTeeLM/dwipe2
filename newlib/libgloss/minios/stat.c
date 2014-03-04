/*
 * Stub version of stat.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#undef errno
extern int errno;

int _stat(const char *file, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

