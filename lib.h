#ifndef __MINIOS_LIB_H__
#define __MINIOS_LIB_H__

#include <stdint.h>
#include <string.h>

struct cpu_ident {
	char type;
	char model;
	char step;
	char fill;
	long cpuid;
	long capability;
	char vend_id[12];
	unsigned char cache_info[16];
	long pwrcap;
	long ext;
	long feature_flag;
	long dcache0_eax;
	long dcache0_ebx;
	long dcache0_ecx;
	long dcache0_edx;
	long dcache1_eax;
	long dcache1_ebx;
	long dcache1_ecx;
	long dcache1_edx;	
	long dcache2_eax;
	long dcache2_ebx;
	long dcache2_ecx;
	long dcache2_edx;	
	long dcache3_eax;
	long dcache3_ebx;
	long dcache3_ecx;
	long dcache3_edx;
};

extern struct cpu_ident cpu_id;
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
/*

static inline void poweroff(void)
{
        asm(
		"movl %cr0,%eax\n\t"
		"andl  $0x00000011,%eax\n\t"
		"orl   $0x60000000,%eax\n\t"
		"movl  %eax,%cr0\n\t"
		"movl  %eax,%cr3\n\t"
		"movl  %cr0,%ebx\n\t"
		"andl  $0x60000000,%ebx\n\t"
		"jz    1f\n\t"
		".byte 0x0f,0x09\n\t"
		"1: andb  $0x10,%al\n\t"
		"movl  %eax,%cr0\n\t"
		"movw $0x0010,%ax\n\t"
		"movw %ax,%ds\n\t"
		"movw %ax,%es\n\t"
		"movw %ax,%fs\n\t"
		"movw %ax,%gs\n\t"
		"movw %ax,%ss\n\t"
		"movw $0x5307, %ax\n\t"
		"movw $0x0001, %bx\n\t"  
		"movw $0x0003, %cx\n\t"
 		"int $0x15\n\t"       
        );
}*/

static inline void reboot(void) 
{
   __asm(
        "movb $0xfe, %al\r\n"
        "outb %al,$0x64"
       );
}
/*
int memcmp(const void *s1, const void *s2, uint32_t count);
void memcpy (void *dst, void *src, int len);
void itoa(char s[], int n);
void reverse(char s[]);
void *memmove(void *dest, const void *src, uint32_t n);
void memset(void *dst, int c, int len);

*/

void set_cache(int val);

//void sleep(int sec);

#endif
