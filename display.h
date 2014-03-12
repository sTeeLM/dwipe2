#ifndef __MINIOS_DISPLAY_H__
#define __MINIOS_DISPLAY_H__

#include "stdint.h"
#include "disk.h"

void cprint(int y, int x, const char *text);
void dprint(int y, int x, uint32_t val, int len, int right);
void hprint(int y, int x, unsigned long val);
void hprint2(int y,int x, unsigned long val, int digits);
void aprint(int y, int x, uint32_t page);
void display_init(void);
#define RES_START	0xa0000
#define RES_END		0x100000
#define SCREEN_ADR	0xb8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_END_ADR  (SCREEN_ADR + SCREEN_WIDTH*SCREEN_HEIGHT*2)

#define LINE_SCROLL	15

//#define NULL	0
#endif
