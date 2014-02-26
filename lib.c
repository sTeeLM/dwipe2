/* lib.c - MemTest-86  Version 3.0
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady, cbrady@sgi.com
 * ----------------------------------------------------
 * MemTest86+ V4.00 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, memtest@memtest.org
 * http://www.canardplus.com - http://www.memtest.org
*/

#include "defs.h"
#include "lib.h"

static inline void cache_off(void)
{
        asm(
		"push %eax\n\t"
		"movl %cr0,%eax\n\t"
                "orl $0x40000000,%eax\n\t"  /* Set CD */
                "movl %eax,%cr0\n\t"
		"wbinvd\n\t"
		"pop  %eax\n\t");
}
static inline void cache_on(void)
{
        asm(
		"push %eax\n\t"
		"movl %cr0,%eax\n\t"
		"andl $0x9fffffff,%eax\n\t" /* Clear CD and NW */
		"movl %eax,%cr0\n\t"
		"pop  %eax\n\t");
}

static inline void reboot(void)
{
        asm(
		"movl %cr0,%eax\n\t"
		"andl  $0x00000011,%eax\n\t"
		"orl   $0x60000000,%eax\n\t"
		"movl  %eax,%cr0\n\t"
		"movl  %eax,%cr3\n\t"
		"movl  %cr0,%ebx\n\t"
		"andl  $0x60000000,%ebx\n\t"
		"jz    f\n\t"
		".byte 0x0f,0x09\n\t"	/* Invalidate and flush cache */
		"f: andb  $0x10,%al\n\t"
		"movl  %eax,%cr0\n\t"
		"movw $0x0010,%ax\n\t"
		"movw %ax,%ds\n\t"
		"movw %ax,%es\n\t"
		"movw %ax,%fs\n\t"
		"movw %ax,%gs\n\t"
		"movw %ax,%ss\n\t"
		"ljmp  $0xffff,$0x0000\n\t");
}

struct cpu_ident cpu_id = {0};


int memcmp(const void *s1, const void *s2, uint32_t count)
{
	const unsigned char *src1 = s1, *src2 = s2;
	int i;
	for(i = 0; i < count; i++) {
		if (src1[i] != src2[i]) {
			return (int)src1[i] - (int)src2[i];
		}
	}
	return 0;
}

void memcpy (void *dst, void *src, int len)
{
	char *s = (char*)src;
	char *d = (char*)dst;
	int i;

	if (len <= 0) {
		return;
	}
	for (i = 0 ; i < len; i++) {
		*d++ = *s++;
	} 
}

void itoa(char s[], int n)
{
	int i, sign;

	if((sign = n) < 0)
		n = -n;
	i=0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	if(sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

void reverse(char s[])
{
	int c, i, j;
	for(j = 0; s[j] != 0; j++)
		;

	for(i=0, j = j - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

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

void *memmove(void *dest, const void *src, uint32_t n)
{
	long i;
	char *d = (char *)dest, *s = (char *)src;

	/* If src == dest do nothing */
	if (dest < src) {
		for(i = 0; i < n; i++) {
			d[i] = s[i];
		}
	}
	else if (dest > src) {
		for(i = n -1; i >= 0; i--) {
			d[i] = s[i];
		}
	}
	return dest;
}

/*
 * Print a people friendly address
 */
void aprint(int y, int x, uint32_t page)
{
	/* page is in multiples of 4K */
	if ((page << 2) < 9999) {
		dprint(y, x, page << 2, 4, 0);
		cprint(y, x+4, "K");
	}
	else if ((page >>8) < 9999) {
		dprint(y, x, (page  + (1 << 7)) >> 8, 4, 0);
		cprint(y, x+4, "M");
	}
	else if ((page >>18) < 9999) {
		dprint(y, x, (page + (1 << 17)) >> 18, 4, 0);
		cprint(y, x+4, "G");
	}
	else {
		dprint(y, x, (page + (1 << 27)) >> 28, 4, 0);
		cprint(y, x+4, "T");
	}
}


void memset(void *dst, int c, int len)
{
    char *d = (char*)dst;
    int i;
	for (i = 0 ; i < len; i++) {
		*d++ = c;
	} 
}

