/*
 * Stub version of write.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

#undef errno
extern int errno;

extern serial_echo(int ch);

#define STDIN_FILENO    0       /* standard input file descriptor */
#define STDOUT_FILENO   1       /* standard output file descriptor */
#define STDERR_FILENO   2       /* standard error file descriptor */

int _write(int file, char *ptr, int len)
{
    int i;
    if ((STDOUT_FILENO == file) || (STDERR_FILENO == file)) {
        for(i = 0 ; i < len; i ++) {
            serial_echo(ptr[i]);
        }
        return len;
    } else {
        errno = EBADF;
        return -1;
    }
}


