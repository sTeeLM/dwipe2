#ifndef __DWIPE_LIB_H__
#define __DWIPE_LIB_H__

#include "stdint.h"
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

int memcmp(const void *s1, const void *s2, uint32_t count);
void memcpy (void *dst, void *src, int len);
void itoa(char s[], int n);
void reverse(char s[]);
void *memmove(void *dest, const void *src, uint32_t n);
void memset(void *dst, int c, int len);
void set_cache(int val);
void sleep(int sec);



#endif
