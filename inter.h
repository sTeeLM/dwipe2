#ifndef __DWIPE_INTER_H__
#define __DWIPE_INTER_H__

#include "stdint.h"

struct eregs {
	uint32_t ss;
	uint32_t ds;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;
	uint32_t vect;
	uint32_t code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflag;
};

#endif