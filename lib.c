#include <sys/time.h>
#include "defs.h"
#include "lib.h"
#include "io.h"



void set_cache(int val)
{
	extern struct cpu_ident cpu_id;
	/* 386's don't have a cache */
	if ((cpu_id.cpuid < 1) && (cpu_id.type == 3)) {
		return;
	}
	switch(val) {
	case 0:
		cache_off();
		break;
	case 1:
		cache_on();
		break;
	}
}



int usleep(useconds_t usec)
{
    struct timeval beg, cur;
    useconds_t diff_us;

    gettimeofday(&beg, NULL); 
    while(1) {
        gettimeofday(&cur, NULL);
        diff_us = (cur.tv_sec - beg.tv_sec) * 1000 * 1000;
        diff_us += (cur.tv_usec - beg.tv_usec);
        if(diff_us >= usec)
            break;
    }
    return 0;    
}

uint32_t msleep(uint32_t msec)
{
    return usleep(msec * 1000);
}

uint32_t sleep(uint32_t sec)
{
    return msleep(sec * 1000);
}



static uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t * rem_p )	
{
    uint64_t quot = 0, qbit = 1;

  if ( den == 0 ) {
    return 1/((unsigned)den); /* Intentional divide by zero, without
                                 triggering a compiler warning which
                                 would abort the build */
  }

  /* Left-justify denominator and count shift */
  while ( (int64_t)den >= 0 ) {
    den <<= 1;
    qbit <<= 1;
  }

  while ( qbit ) {
    if ( den <= num ) {
      num -= den;
      quot += qbit;
    }
    den >>= 1;
    qbit >>= 1;
  }

  if ( rem_p )
    *rem_p = num;

  return quot;
}

uint64_t __udivdi3(uint64_t num, uint64_t den)
{
   return __udivmoddi4(num, den, NULL);
}

uint64_t __umoddi3(uint64_t num, uint64_t den)
{
    uint64_t v=0;
    (void) __udivmoddi4(num, den, &v);
    return v;
}
