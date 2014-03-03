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
#include "io.h"

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

/*
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
*/
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

/*
void memset(void *dst, int c, int len)
{
    char *d = (char*)dst;
    int i;
	for (i = 0 ; i < len; i++) {
		*d++ = c;
	} 
}
*/
/*
Register  Contents
 0x00      Seconds
 0x02      Minutes
 0x04      Hours
 0x06      Weekday
 0x07      Day of Month
 0x08      Month
 0x09      Year
 0x32      Century (maybe)
 0x0A      Status Register A
 0x0B      Status Register B
*/
struct cmos_content
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t day_of_month;
    uint8_t month;
    uint8_t year;
};
void read_cmos(struct cmos_content * cmos)
{
    uint8_t registerB;

    outb(0, 0x70);
    cmos->seconds = inb(0x71);

    outb(0x2, 0x70);
    cmos->minutes = inb(0x71);

    outb(0x4, 0x70);
    cmos->hours = inb(0x71);

    outb(0x6, 0x70);
    cmos->weekday = inb(0x71);

    outb(0x7, 0x70);
    cmos->day_of_month = inb(0x71);

    outb(0x8, 0x70);
    cmos->month = inb(0x71);

    outb(0x9, 0x70);
    cmos->year = inb(0x71);

    outb(0xB, 0x70);
    registerB = inb(0x71);

    // Convert BCD to binary values if necessary
    if (!(registerB & 0x04)) {
        cmos->seconds = (cmos->seconds & 0x0F) + ((cmos->seconds / 16) * 10);
        cmos->minutes = (cmos->minutes & 0x0F) + ((cmos->minutes / 16) * 10);
        cmos->hours = ( (cmos->hours & 0x0F) + (((cmos->hours & 0x70) / 16) * 10) ) | (cmos->hours & 0x80);
        cmos->day_of_month = (cmos->day_of_month & 0x0F) + ((cmos->day_of_month / 16) * 10);
        cmos->month = (cmos->month & 0x0F) + ((cmos->month / 16) * 10);
        cmos->year = (cmos->year & 0x0F) + ((cmos->year / 16) * 10);
    }
}


void sleep(int sec)
{
   
    struct cmos_content cmos, cmos2;
    memset(&cmos, 0, sizeof(cmos));
    memset(&cmos, 0, sizeof(cmos2));
    read_cmos(&cmos);
    memcpy(&cmos2, &cmos, sizeof(cmos));
    while(1) {
        read_cmos(&cmos2);
        if(cmos2.seconds - sec > cmos.seconds)
            break;
    }
    return;
    
}

