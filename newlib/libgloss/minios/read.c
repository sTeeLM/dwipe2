/*
 * Stub version of read.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

#undef errno
extern int errno;

#define STDIN_FILENO    0       /* standard input file descriptor */
#define STDOUT_FILENO   1       /* standard output file descriptor */
#define STDERR_FILENO   2       /* standard error file descriptor */

int _read(int file, char *ptr, int len)
{

    if ((STDOUT_FILENO == file) || (STDERR_FILENO == file)) {
        return 0;
    } else {
        errno = EBADF;
        return -1;
    }

}

