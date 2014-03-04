/*
 * Stub version of fstat.
 */

#include "config.h"
#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


#undef errno
extern int errno;

#define STDIN_FILENO    0       /* standard input file descriptor */
#define STDOUT_FILENO   1       /* standard output file descriptor */
#define STDERR_FILENO   2       /* standard error file descriptor */

int _fstat(int fildes, struct stat * st)
{
    if ((STDOUT_FILENO == fildes) || (STDERR_FILENO == fildes)) {
        st->st_mode = S_IFCHR;
        return 0;
    } else {
        errno = EBADF;
        return -1;
    }
}

