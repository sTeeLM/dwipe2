/*
 * Stub version of gettimeofday.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#undef errno
extern int errno;

struct timeval;

int _gettimeofday(struct timeval  *ptimeval, void *ptimezone)
{
  errno = ENOSYS;
  return -1;
}

