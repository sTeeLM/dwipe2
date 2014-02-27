#ifndef _DWIPE_DISK_H__
#define _DWIPE_DISK_H__

#include "stdint.h"
#include "defs.h"


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


struct disk_dap
{
	uint8_t  size;          // size of DAP = 16 = 10h
	uint8_t  unused;        // unused, should be zero
	uint16_t nsectors;		// number of sectors to be read
	uint16_t offset;        // offset of buffer
	uint16_t base;          // base of buffer
	uint64_t begin;         // absolute number of the start of the sectors to be read (1st sector of drive has number 0)    
}__attribute__((packed));


struct disk_param_ext
{
    uint16_t  size;           // size of Result Buffer = 30 = 1Eh
    uint16_t  flags;          // information flags
    uint32_t  ncylinders;     // physical number of cylinders = last index + 1 (because index starts with 0)
    uint32_t  nheads;         // physical number of heads = last index + 1 (because index starts with 0)
    uint32_t  sectors_per_track;// physical number of sectors per track = last index (because index starts with 1)
    uint64_t  nsectors;       //absolute number of sectors = last index + 1 (because index starts with 0)
    uint16_t  bytes_per_sector; //bytes per sector
    uint32_t  edd;              //optional pointer to Enhanced Disk Drive (EDD) configuration parameters
}__attribute__((packed));


#define CHS_HEAD_MAX 254
/*
The CHS addressing supported in IBM-PC compatible BIOSes code used eight bits for - 
theoretically up to 256 heads counted as head 0 up to 255 (FFh). However, 
a bug in all versions of MS-DOS/PC DOS up to including 7.10 will cause these 
operating systems to crash on boot when encountering volumes with 256 heads. 
Therefore, all compatible BIOSes will use mappings with up to 255 heads (00h..FEh) only, 
including in virtual 255��63 geometries.
*/
#define CHS_CYLINDER_MAX 1023
#define CHS_SECTOR_MAX  63
struct disk_param {
    uint8_t   disk_id;
    uint8_t   drives;         // number of hard disk drives
    uint8_t   last_heads;     // logical last index of heads = number_of - 1 (because index starts with 0), max 2^8 - 1 = 255, due bug, max 254
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
int write_sectors_chs(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size, int check);
int read_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t * size);
int write_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t * size, int check);
int check_extensions_present(uint8_t disk_id, int * ext, int * version);
int lba2chs(struct disk_param *, uint64_t lba, int * cylinder, int * head, int * sector );
int chs2lba(struct disk_param *, int cylinder, int head, int sector ,uint64_t * lba);
void relocate_real(void);

extern char * p_real_buffer;
extern struct reg_req * p_real_reg;
extern struct disk_dap * p_real_dap;
extern struct disk_param_ext * p_real_disk_param_ext;
extern uint32_t * p_real_ret;
#endif
