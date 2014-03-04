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

extern void do_gettimeofday(struct timeval *tv);

int _gettimeofday(struct timeval  *ptimeval, void *ptimezone)
{
    do_gettimeofday(ptimeval);
    return 0;
}

