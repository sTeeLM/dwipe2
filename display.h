#ifndef __DWIPE_DISPLAY_H__
#define __DWIPE_DISPLAY_H__

#include "stdint.h"

void cprint(int y, int x, const char *text);
void dprint(int y, int x, uint32_t val, int len, int right);
void hprint(int y, int x, unsigned long val);
void hprint2(int y,int x, unsigned long val, int digits);
void aprint(int y, int x, uint32_t page);
void display_init(void);

#define RES_START	0xa0000
#define RES_END		0x100000
#define SCREEN_ADR	0xb8000
#define SCREEN_END_ADR  (SCREEN_ADR + 80*25*2)

#define TITLE_WIDTH	28
#define LINE_TIME	11
#define COL_TIME	0
#define LINE_TST	2
#define LINE_RANGE	3
#define LINE_CPU	1
#define COL_MID		30
#define LINE_PAT  4
#define COL_PAT		41
#define LINE_INFO	11
#define COL_CACHE_TOP   13
#define COL_RESERVED    22
#define COL_MMAP	29
#define COL_CACHE	40
#define COL_ECC		46
#define COL_TST		51
#define COL_PASS	56
#define COL_ERR		63
#define COL_ECC_ERR	72
#define LINE_HEADER	13
#define LINE_SCROLL	15
#define BAR_SIZE	(78-COL_MID-9)
#define LINE_MSG	18
#define COL_MSG		18

#define POP_W	30
#define POP_H	15
#define POP_X	16
#define POP_Y	8
#define POP2_W	74
#define POP2_H	21
#define POP2_X	3
#define POP2_Y	2
//#define NULL	0
#endif
