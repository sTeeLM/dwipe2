#ifndef _DWIPE_DISK_H__
#define _DWIPE_DISK_H__

#include "dwipe.h"
#include "stdint.h"
#include "defs.h"

extern char * real_buffer;
extern int int13_1(void);
extern int int13_2(void);
extern int int13_3(void);
extern int int13_4(void);
struct reg_req
{
    uint8_t al;
    uint8_t ah;
    uint8_t bl;
    uint8_t bh;
    uint8_t cl;
    uint8_t ch;
    uint8_t dl;
    uint8_t dh;
    uint16_t es;
    uint16_t si;
}__attribute__((packed));

extern struct reg_req real_reg;

struct disk_dap
{
	uint8_t  size;          // size of DAP = 16 = 10h
	uint8_t  unused;        // unused, should be zero
	uint16_t nsectors;		// number of sectors to be read
	uint16_t offset;        // offset of buffer
	uint16_t base;          // base of buffer
	uint64_t begin;         // absolute number of the start of the sectors to be read (1st sector of drive has number 0)    
}__attribute__((packed));

extern struct disk_dap real_dap;


struct disk_param_ext
{
    uint16_t  size;           // size of Result Buffer = 30 = 1Eh
    uint16_t  flags;          // information flags
    uint32_t  ncylinders;     // physical number of cylinders = last index + 1 (because index starts with 0)
    uint32_t  nheads;         // physical number of heads = last index + 1 (because index starts with 0)
    uint32_t  sectors_per_track;// physical number of sectors per track = last index (because index starts with 1)
    uint64_t  nsectors;       //absolute number of sectors = last index + 1 (because index starts with 0)
    uint16_t  bytes_per_sector; //bytes per sector
}__attribute__((packed));

extern struct disk_param_ext real_param_ext;

#define CHS_HEAD_MAX 255
#define CHS_CYLINDER_MAX 1023
#define CHS_SECTOR_MAX  63
struct disk_param {
    uint8_t   disk_id;
    uint8_t   drives;         // number of hard disk drives
    uint8_t   last_heads;     // logical last index of heads = number_of - 1 (because index starts with 0), max 2^8 - 1 = 255
    uint16_t  last_cylinders; // logical last index of cylinders = number_of - 1 (because index starts with 0), max 2^10 - 1 = 1023
    uint8_t   last_sectors_per_track;   // logical last index of sectors per track = number_of (because index starts with 1), max 2^6 - 1 = 63
    uint8_t   drive_type;     // drive type (only AT/PS2 floppies)
    uint8_t   has_ext;        // is ext support?
    struct disk_param_ext ext;
};

int get_disk_param(uint8_t disk_id, struct disk_param * param);
int get_disk_param_ext(struct disk_param * param);
int reset_disk_drive(uint8_t disk_id);
int get_last_status(uint8_t disk_id, int * status);
int read_sectors_chs(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size);
int read_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t * size);
int write_sectors_chs(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size);
int write_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t * size, int check);
int check_extensions_present(uint8_t disk_id, int * ext, int * version);
#endif
