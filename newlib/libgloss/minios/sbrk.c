/* Version of sbrk for no operating system.  */

#include "config.h"
#include <_syslist.h>

void * _sbrk (int incr)
{ 
   extern char * _end; /* Set by linker.  */
   static char * heap_end; 
   char *        prev_heap_end; 


   return (void *) prev_heap_end; 
} 
