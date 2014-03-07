#include "disk.h"
#include "lib.h"
#include "debug.h"
#include <inttypes.h>

extern char real_code_begin[];
extern char real_code_end[];

/* should be relocate to p_xx*/
extern char real_buffer[MAX_SECTOR_BUFFER_SIZE];
extern reg_req_t real_reg;
extern struct disk_dap real_dap;
extern struct disk_param_ext real_disk_param_ext;

char * p_real_buffer;
reg_req_t * p_real_reg;
struct disk_dap * p_real_dap;
struct disk_param_ext * p_real_disk_param_ext;

extern int int13_1(void);
extern int int13_2(void);
extern int int13_3(void);
extern int int13_4(void);


void relocate_real()
{
    uint32_t offset = (uint32_t)real_code_begin - (uint32_t)REAL_CODE_ADDR;

    SDBG("real_code_begin = %p, real_code_end = %p", real_code_begin, real_code_end);
    SDBG("offset = %x", offset);

    /* relocate code */
    char * dst = (char *)REAL_CODE_ADDR;
    memcpy(dst, real_code_begin, real_code_end - real_code_begin);

    /* relocate buffer */
    p_real_buffer = real_buffer;
    p_real_reg    = &real_reg;
    p_real_dap    = &real_dap;
    p_real_disk_param_ext = &real_disk_param_ext;

    SDBG("before relocate");
    SDBG("p_real_buffer = %p", p_real_buffer);
    SDBG("p_real_reg = %p", p_real_reg);
    SDBG("p_real_dap = %p", p_real_dap);
    SDBG("p_real_disk_param_ext = %p", p_real_disk_param_ext);


    p_real_buffer = (char *)p_real_buffer - offset;
    p_real_reg    = (reg_req_t *)((char *)p_real_reg - offset);
    p_real_dap    = (struct disk_dap *)((char *)p_real_dap - offset);
    p_real_disk_param_ext = (struct disk_param_ext *)((char *)p_real_disk_param_ext - offset);

    SDBG("after relocate");
    SDBG("p_real_buffer = %p", p_real_buffer);
    SDBG("p_real_reg = %p", p_real_reg);
    SDBG("p_real_dap = %p", p_real_dap);
    SDBG("p_real_disk_param_ext = %p", p_real_disk_param_ext);
}

/*
CX	[7:6] [15:8] logical last index of cylinders = number_of - 1 (because index starts with 0)
[5:0] logical last index of sectors per track = number_of (because index starts with 1)
CX := ( ( cylinder and 255 ) shl 8 ) or ( ( cylinder and 768 ) shr 2 ) or sector;
cylinder := ( (CX and 0xFF00) shr 8 ) or ( (CX and 0xC0) shl 2)
sector := CX and 63;
*/

void get_chs_from_pack(uint16_t c_s, int * cylinder, int * sector)
{
    *cylinder = ( (c_s & 0xFF00) >> 8 ) | ( (c_s & 0xC0) << 2);
    *sector  = c_s & 63;
}

void set_chs_to_pack(uint16_t * c_s, int cylinder, int sector)
{
    *c_s = ( ( cylinder & 255 ) << 8 ) | ( ( cylinder & 768 ) >> 2 ) | sector;
}

/*
INT 13h AH=08h: Read Drive Parameters[edit]
Parameters:
Registers
AH	08h = function number for read_drive_parameters
DL	drive index (e.g. 1st HDD = 80h)
ES:DI[4]	set to 0000h:0000h to work around some buggy BIOS

Results:
CF	Set On Error, Clear If No Error
AH	Return Code
DL	number of hard disk drives
DH	logical last index of heads = number_of - 1 (because index starts with 0)
CX	[7:6] [15:8] logical last index of cylinders = number_of - 1 (because index starts with 0)
[5:0] logical last index of sectors per track = number_of (because index starts with 1)
BL	drive type (only AT/PS2 floppies)
ES:DI	pointer to drive parameter table (only for floppies)
*/
int get_disk_param(uint8_t disk_id, struct disk_param * param)
{
    int ret;
    int c, s;

    memset(param, 0, sizeof(struct disk_param));
    
    p_real_reg->h.al = 0;
    p_real_reg->h.ah = 0x08;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = disk_id; 
    ret = int13_1();
    if(ret == 0) {
        param->disk_id = disk_id;
        param->drives = p_real_reg->h.dl;
        param->last_heads = p_real_reg->h.dh;
        get_chs_from_pack(p_real_reg->w.cx, &c, &s);
        param->last_sectors_per_track = s;
        param->last_cylinders = c;
        param->drive_type = p_real_reg->h.bl;
        param->has_ext = 0;
        memset(&param->ext, 0, sizeof(param->ext));
    }

    return ret == 0 ? ret : p_real_reg->h.ah;
}

/*
INT 13h AH=48h: Extended Read Drive Parameters[edit]
Parameters:
Registers
AH	48h = function number for extended_read_drive_parameters
DL	drive index (e.g. 1st HDD = 80h)
DS:SI	segment:offset pointer to Result Buffer, see below
Result Buffer
offset range	size	description
00h..01h	2 bytes	size of Result Buffer = 30 = 1Eh
02h..03h	2 bytes	information flags
04h..07h	4 bytes	physical number of cylinders = last index + 1 (because index starts with 0)
08h..0Bh	4 bytes	physical number of heads = last index + 1 (because index starts with 0)
0Ch..0Fh	4 bytes	physical number of sectors per track = last index (because index starts with 1)
10h..17h	8 bytes	absolute number of sectors = last index + 1 (because index starts with 0)
18h..19h	2 bytes	bytes per sector
1Ah..1Dh	4 bytes	optional pointer to Enhanced Disk Drive (EDD) configuration parameters
which may be used for subsequent interrupt 13h Extension calls (if supported)
Results:
CF	Set On Error, Clear If No Error
AH	Return Code
*/
int get_disk_param_ext(struct disk_param * param)
{
    int ret;
    p_real_reg->h.al = 0;
    p_real_reg->h.ah = 0x48;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = param->disk_id;

    ret = int13_4();
    if(ret == 0) {
        param->has_ext = 1;
        memcpy(&param->ext, p_real_disk_param_ext, sizeof(param->ext));
    }

    return ret == 0 ? ret : p_real_reg->h.ah;
}

/*
INT 13h AH=00h: Reset Disk Drive[edit]
Parameters:
AH	00h
DL	Drive
Results:
CF	Set on error
*/
int reset_disk_drive(uint8_t disk_id)
{
    int ret;
    p_real_reg->h.al = 0;
    p_real_reg->h.ah = 0x0;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = disk_id;
    
    ret = int13_1();

    return ret;
}

/*
INT 13h AH=01h: Get Status of Last Drive Operation[edit]
Parameters:
AH	01h
DL	Drive$
$Bit 7=0 for floppy drive, bit 7=1 for fixed drive
Results:
AL	Return Code
00h	Success
01h	Invalid Command
02h	Cannot Find Address Mark
03h	Attempted Write On Write Protected Disk
04h	Sector Not Found
05h	Reset Failed
06h	Disk change line 'active'
07h	Drive parameter activity failed
08h	DMA overrun
09h	Attempt to DMA over 64kb boundary
0Ah	Bad sector detected
0Bh	Bad cylinder (track) detected
0Ch	Media type not found
0Dh	Invalid number of sectors
0Eh	Control data address mark detected
0Fh	DMA out of range
10h	CRC/ECC data error
11h	ECC corrected data error
20h	Controller failure
40h	Seek failure
80h	Drive timed out, assumed not ready
AAh	Drive not ready
BBh	Undefined error
CCh	Write fault
E0h	Status error
FFh	Sense operation failed
CF	Set On Error, Clear If No Error
*/

int get_last_status(uint8_t disk_id, int * status)
{
    int ret;
    p_real_reg->h.al = 0;
    p_real_reg->h.ah = 0x01;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = disk_id;

    ret = int13_1();

    *status = p_real_reg->h.al;

    return ret;
}

static int varify_chs(int cylinder, int head, int sector)
{
    if(cylinder > CHS_CYLINDER_MAX 
        || cylinder < 0
        || head > CHS_HEAD_MAX 
        || head < 0
        || sector > CHS_SECTOR_MAX
        || sector <= 0)
        return -1;
    return 0;
}

static int sector_chs_verify(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t size)
{
    int ret;
    int sector_size = 512;

    if(varify_chs(cylinder, head, sector) < 0)
        return -1;

    if(param->has_ext) {
        sector_size = param->ext.bytes_per_sector;
    }

    if(nsector > BIOS_MAX_SECTOR_CNT)
        return -1;

    if(nsector * sector_size > size)
        return -1;

    if(nsector * sector_size > MAX_SECTOR_BUFFER_SIZE)
        return -1;

     if(cylinder < 0 
        || cylinder > param->last_cylinders
        || head < 0
        || head > param->last_heads  
        || sector <=0
        || sector > param->last_sectors_per_track) {
        return -1;
    }

    p_real_reg->h.dh = head;
    set_chs_to_pack(&p_real_reg->w.cx, cylinder, sector);
    p_real_reg->h.al = nsector;
    p_real_reg->h.ah = 0x04;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.dl = param->disk_id;
    ret = int13_2();

    return ret == 0 ? ret : p_real_reg->h.ah;
}

static int sector_chs_rw(int is_write, struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size, int check)
{
    int ret;
    int sector_size = 512;

    if(varify_chs(cylinder, head, sector) < 0)
        return -1;

    if(param->has_ext) {
        sector_size = param->ext.bytes_per_sector;
    }

    if(nsector > BIOS_MAX_SECTOR_CNT)
        return -1;


    if(nsector * sector_size > *size)
        return -1;


    if(nsector * sector_size > MAX_SECTOR_BUFFER_SIZE)
        return -1;


    if(is_write && *size > MAX_SECTOR_BUFFER_SIZE) {
        return -1;
    }


    if(cylinder < 0 
        || cylinder > param->last_cylinders
        || head < 0
        || head > param->last_heads  
        || sector <=0
        || sector > param->last_sectors_per_track) {
        return -1;
    }


    if(is_write) {
        memcpy(p_real_buffer, buffer, *size );
    }
    p_real_reg->h.dh = head;
    set_chs_to_pack(&p_real_reg->w.cx, cylinder, sector);
    p_real_reg->h.al = nsector;
    p_real_reg->h.ah = is_write ? 0x03 : 0x02;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.dl = param->disk_id;

    ret = int13_2();

    if(ret == 0) {
        if(!is_write) {
            memcpy(buffer, p_real_buffer, (p_real_reg->h.al > nsector ? nsector : p_real_reg->h.al) * sector_size );
        }
        *size = (p_real_reg->h.al > nsector ? nsector : p_real_reg->h.al) * sector_size;

        if(check && is_write) {
            ret = sector_chs_verify(param, cylinder, head, sector, nsector, buffer, *size);
        }
    }

    return ret == 0 ? ret : p_real_reg->h.ah;
}

/*
INT 13h AH=02h: Read Sectors From Drive[edit]
Parameters:
AH	02h
AL	Sectors To Read Count
CH	Cylinder
CL	Sector
DH	Head
DL	Drive
ES:BX	Buffer Address Pointer
Results:
CF	Set On Error, Clear If No Error
AH	Return Code
AL	Actual Sectors Read Count

*/
int read_sectors_chs(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size)
{
    return sector_chs_rw(0, param, cylinder, head, sector, nsector, buffer, size, 0);
}


static int sectors_lba_rw(int is_write, struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t *size, int check)
{
    int ret;
    if(!param->has_ext
        || offset > param->ext.nsectors - 1 )
 //       || nsector + offset > param->ext.nsectors)
        return -1;
    
    if(nsector > BIOS_MAX_SECTOR_CNT)
        return -1;

    if(nsector * param->ext.bytes_per_sector > MAX_SECTOR_BUFFER_SIZE)
        return -1;

    if(nsector * param->ext.bytes_per_sector > *size)
        return -1;

    if(is_write && *size > MAX_SECTOR_BUFFER_SIZE) {
        return -1;
    }

    if(is_write) {
        memcpy(p_real_buffer, buffer, *size );
    }

    p_real_dap->nsectors = nsector;
    p_real_dap->begin = offset;

    p_real_reg->h.al = check;
    p_real_reg->h.ah = is_write ? 0x43 : 0x42;
    p_real_reg->h.bl = 0;
    p_real_reg->h.bh = 0;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = param->disk_id;

    ret = int13_3();

    if(ret == 0) {
        if(!is_write) {
            memcpy(buffer, p_real_buffer, (p_real_dap->nsectors > nsector ? nsector : p_real_dap->nsectors) * param->ext.bytes_per_sector );
        }
        *size = (p_real_reg->h.al > nsector ? nsector : p_real_dap->nsectors) * param->ext.bytes_per_sector;
    }

    return ret == 0 ? ret : p_real_reg->h.ah;
}

/*
INT 13h AH=42h: Extended Read Sectors From Drive[edit]
Parameters:
Registers
AH	42h = function number for extended read
DL	drive index (e.g. 1st HDD = 80h)
DS:SI	segment:offset pointer to the DAP, see below
DAP : Disk Address Packet
offset range	size	description
00h	1 byte	size of DAP = 16 = 10h
01h	1 byte	unused, should be zero
02h..03h	2 bytes	number of sectors to be read, (some Phoenix BIOSes are limited to a maximum of 127 sectors)
04h..07h	4 bytes	segment:offset pointer to the memory buffer to which sectors will be transferred (note that x86 is little-endian: if declaring the segment and offset separately, the offset must be declared before the segment)
08h..0Fh	8 bytes	absolute number of the start of the sectors to be read (1st sector of drive has number 0)
Results:
CF	Set On Error, Clear If No Error
AH	Return Code

*/
int read_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t *size)
{
    return sectors_lba_rw(0, param, offset, nsector, buffer, size, 0);
}

/*
INT 13h AH=03h: Write Sectors To Drive[edit]
Parameters:
AH	03h
AL	Sectors To Write Count
CH	Track
CL	Sector
DH	Head
DL	Drive
ES:BX	Buffer Address Pointer
Results:
CF	Set On Error, Clear If No Error
AH	Return Code
AL	Actual Sectors Written Count
*/
int write_sectors_chs(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size, int check)
{
    return sector_chs_rw(1, param, cylinder, head, sector, nsector, buffer, size, check);
}

/*
INT 13h AH=43h: Extended Write Sectors to Drive[edit]
Parameters:
Registers
AH	43h = function number for extended write
AL	bit 0 = 0: close write check,
bit 0 = 1: open write check,
bit 1-7:reserved, set to 0
DL	drive index (e.g. 1st HDD = 80h)
DS:SI	segment:offset pointer to the DAP
Results:
CF	Set On Error, Clear If No Error
AH	Return Code

*/
int write_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsector, void * buffer, uint32_t * size, int check)
{
    return sectors_lba_rw(1, param, offset, nsector, buffer, size, check);
}

/*
INT 13h AH=41h: Check Extensions Present[edit]
Parameters:
Registers
AH	41h = function number for extensions check[5]
DL	drive index (e.g. 1st HDD = 80h)
BX	55AAh
Results:
CF	Set On Not Present, Clear If Present
AH	Error Code or Major Version Number
BX	AA55h
CX	Interface support bitmask:
1 - Device Access using the packet structure
2 - Drive Locking and Ejecting
4 - Enhanced Disk Drive Support (EDD)
*/
int check_extensions_present(uint8_t disk_id, int * ext, int * version)
{
    int ret;
    p_real_reg->h.al = 0;
    p_real_reg->h.ah = 0x41;
    p_real_reg->h.bl = 0xaa;
    p_real_reg->h.bh = 0x55;
    p_real_reg->h.ch = 0;
    p_real_reg->h.cl = 0;
    p_real_reg->h.dh = 0;
    p_real_reg->h.dl = disk_id;

    ret = int13_1();
    if(ret == 0) {
        *ext = p_real_reg->h.cl;
        *version = p_real_reg->h.ah;
    }

    return ret == 0 ? ret : p_real_reg->h.ah;

}
/*
    cylinder = LBA / (SPT * HPC)
    head = (LBA / SPT) % HPC
    sector = LBA % SPT + 1
*/
int lba2chs(struct disk_param * param, uint64_t lba, int * cylinder, int * head, int * sector )
{
    /* Function 02h of interrupt 13h may only read sectors of the first 16,450,560 sectors of your hard drive */
    uint32_t max_lba = (CHS_CYLINDER_MAX * (CHS_HEAD_MAX + 1) + CHS_HEAD_MAX) * (CHS_SECTOR_MAX) + CHS_SECTOR_MAX - 1;
    uint32_t new_val = 0;
    if(lba <= max_lba) {
        new_val = (uint32_t)lba;
        *head = (new_val / (param->last_sectors_per_track)) % (param->last_heads + 1);
        *cylinder = new_val / ((param->last_sectors_per_track) * (param->last_heads + 1));
        *sector = new_val % (param->last_sectors_per_track) + 1;
        return 0;
    }
    return -1;
}

/*
    LBA = (cylinder * HPC + head) * SPT + sector - 1
*/
int chs2lba(struct disk_param * param, int cylinder, int head, int sector ,uint64_t * lba)
{
    if(varify_chs(cylinder, head, sector) < 0)
        return -1;
    *lba = (cylinder * (param->last_heads + 1) + head) *( param->last_sectors_per_track) + sector - 1;
    return 0;
}

void dump_disk(struct disk_param * param)
{
    SDBG("----------start dump disk--------");
    SDBG("++id: %x", param->disk_id);
    SDBG("++drives: %d", param->drives);
    SDBG("++last_heads: %d", param->last_heads);
    SDBG("++last_cylinders: %d", param->last_cylinders);
    SDBG("++last_sectors_per_track: %d", param->last_sectors_per_track);
    SDBG("++drive_type: %d", param->drive_type);
    SDBG("++has_ext: %d", param->has_ext);
    SDBG("++ext size: %d", param->ext.size);
    SDBG("++ext flags: %d", param->ext.flags);
    SDBG("++ext ncylinders: %d", param->ext.ncylinders);
    SDBG("++ext nheads: %d", param->ext.nheads);
    SDBG("++ext sectors_per_track: %d", param->ext.sectors_per_track);
    SDBG("++ext nsectors: %" PRIu64 , param->ext.nsectors);
    SDBG("++ext bytes_per_sector: %d", param->ext.bytes_per_sector);
    SDBG("++ext edd: %x", param->ext.edd);
    SDBG("----------end dump disk--------");
}