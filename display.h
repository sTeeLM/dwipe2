#ifndef __DWIPE_DISPLAY_H__
#define __DWIPE_DISPLAY_H__

#include "stdint.h"
#include "disk.h"

void cprint(int y, int x, const char *text);
void dprint(int y, int x, uint32_t val, int len, int right);
void hprint(int y, int x, unsigned long val);
void hprint2(int y,int x, unsigned long val, int digits);
void aprint(int y, int x, uint32_t page);
void display_init(void);

void display_cpu(const char * cpu, int l1, int l2, int l3, uint32_t speed_kh);
void display_mode();
void display_disk_list(uint32_t total, uint32_t wiping, uint32_t wiped);
void display_disk_status(struct disk_param *disk);
void display_progress(int progress, uint64_t write_bytes);
#define RES_START	0xa0000
#define RES_END		0x100000
#define SCREEN_ADR	0xb8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_END_ADR  (SCREEN_ADR + SCREEN_WIDTH*SCREEN_HEIGHT*2)

#define LINE_CPU	2
#define LINE_MODE	4
#define LINE_DISK_LIST 6
#define LINE_WIPE_STATUS 8

#define LINE_PROGRESS (LINE_WIPE_STATUS + 5)

#define LINE_SCROLL	15

//#define NULL	0
#endif
