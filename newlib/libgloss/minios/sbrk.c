/* Version of sbrk for no operating system.  */

#include "config.h"
#include <_syslist.h>
#include <errno.h>

#undef errno
extern int errno;

#define MAX_HEAP_SIZE 10485760 // 10MB

extern char _end[];
static char * heap_ptr;

void * _sbrk (int nbytes)
{ 

    if(heap_ptr == 0)
        heap_ptr = _end;

    if ( (heap_ptr - _end + nbytes) < MAX_HEAP_SIZE )
    {
        void *base = heap_ptr;
        heap_ptr += nbytes;
        return base;
    }
    else
    {
        errno = ENOMEM;
        return (void *) -1;
    }

} 
