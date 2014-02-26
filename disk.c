#include "disk.h"
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
    uint32_t cx = 0;

    memset(param, 0, sizeof(struct disk_param));
    
    real_reg.al = 0;
    real_reg.ah = 0x08;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = disk_id; 
    ret = int13_1();
    if(ret == 0) {
        param->disk_id = disk_id;
        param->drives = real_reg.dl;
        param->last_heads = real_reg.dh;
        cx = real_reg.cl;
        param->last_sectors_per_track = (uint8_t)(real_reg.cl & 0x000003f);
        cx <<=2;
        cx &=0x300;
        cx |= real_reg.ch;
        param->last_cylinders = (uint16_t) (cx & 0x3ff);
        param->drive_type = real_reg.bl;

        param->has_ext = 0;
        memset(&param->ext, 0, sizeof(param->ext));
    }

    return ret == 0 ? ret : real_reg.ah;
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
    real_reg.al = 0;
    real_reg.ah = 0x48;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = param->disk_id;

    ret = int13_4();
    if(ret == 0) {
        param->has_ext = 1;
        memcpy(&param->ext, &real_disk_param_ext, sizeof(param->ext));
    }

    return ret == 0 ? ret : real_reg.ah;
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
    real_reg.al = 0;
    real_reg.ah = 0x0;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = disk_id;
    
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
    real_reg.al = 0;
    real_reg.ah = 0x01;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = disk_id;

    ret = int13_1();

    *status = real_reg.al;

    return ret;
}

static int sector_chs_verify(struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t size)
{
    int ret;
    int cx = 0;
    int sector_size = 512;

    if(cylinder > CHS_CYLINDER_MAX || head > CHS_HEAD_MAX || sector > CHS_SECTOR_MAX)
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
    cx = cylinder;
    cx >>=2;
    cx &=0xc0;
    cx |= (sector & 0x3f);
    real_reg.cl = cx & 0xff;

    real_reg.ch = cylinder & 0xff;

    real_reg.al = nsector;
    real_reg.ah = 0x04;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.dh = head;
    real_reg.dl = param->disk_id;

    ret = int13_2();

    return ret == 0 ? ret : real_reg.ah;
}

static int sector_chs_rw(int is_write, struct disk_param * param, int cylinder, int head, int sector, uint32_t nsector, void * buffer, uint32_t * size, int check)
{
    int ret;
    int cx = 0;
    int sector_size = 512;

    if(cylinder > CHS_CYLINDER_MAX || head > CHS_HEAD_MAX || sector > CHS_SECTOR_MAX)
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
        memcpy(real_buffer, buffer, *size );
    }

    cx = cylinder;
    cx >>=2;
    cx &=0xc0;
    cx |= (sector & 0x3f);
    real_reg.cl = cx & 0xff;

    real_reg.ch = cylinder & 0xff;

    real_reg.al = nsector;
    real_reg.ah = is_write ? 0x03 : 0x02;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.dh = head;
    real_reg.dl = param->disk_id;

    ret = int13_2();

    if(ret == 0) {
        if(!is_write) {
            memcpy(buffer, real_buffer, (real_reg.al > nsector ? nsector : real_reg.al) * sector_size );
        }
        *size = (real_reg.al > nsector ? nsector : real_reg.al) * sector_size;

        if(check) {
            ret = sector_chs_verify(param, cylinder, head, sector, nsector, buffer, *size);
        }
    }

    return ret == 0 ? ret : real_reg.ah;
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
        || offset > param->ext.nsectors - 1 
        || nsector + offset > param->ext.nsectors)
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
        memcpy(real_buffer, buffer, *size );
    }

    real_dap.nsectors = nsector;
    real_dap.begin = offset;

    real_reg.al = check;
    real_reg.ah = is_write ? 0x43 : 0x42;
    real_reg.bl = 0;
    real_reg.bh = 0;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = param->disk_id;

    ret = int13_3();

    if(ret == 0) {
        if(!is_write) {
            memcpy(buffer, real_buffer, (real_dap.nsectors > nsector ? nsector : real_dap.nsectors) * param->ext.bytes_per_sector );
        }
        *size = (real_reg.al > nsector ? nsector : real_dap.nsectors) * param->ext.bytes_per_sector;
    }

    return ret == 0 ? ret : real_reg.ah;
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
    real_reg.al = 0;
    real_reg.ah = 0x41;
    real_reg.bl = 0xaa;
    real_reg.bh = 0x55;
    real_reg.ch = 0;
    real_reg.cl = 0;
    real_reg.dh = 0;
    real_reg.dl = disk_id;

    ret = int13_1();
    if(ret == 0) {
        *ext = real_reg.cl;
        *version = real_reg.ah;
    }

    return ret == 0 ? ret : real_reg.ah;

}
