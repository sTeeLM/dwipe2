#include <stdint.h>
#include <string.h>

#include "fat.h"
#include "disk.h"
#include "option.h"
#include "debug.h"

#define FAT_LABEL_LEN 11

struct fat_boot_sector_32 {
    uint8_t ignored[3];		/* Boot strap short or near jump */
    uint8_t system_id[8];		/* Name - can be used to special case
				   partition manager volumes */
    uint8_t sector_size[2];	/* bytes per logical sector */
    uint8_t cluster_size;		/* sectors/cluster */
    uint16_t reserved;		/* reserved sectors */
    uint8_t fats;			/* number of FATs */
    uint8_t dir_entries[2];	/* root directory entries */
    uint8_t sectors[2];		/* number of sectors */
    uint8_t media;			/* media code (unused) */
    uint16_t fat_length;		/* sectors/FAT */
    uint16_t secs_track;		/* sectors per track */
    uint16_t heads;		/* number of heads */
    uint32_t hidden;		/* hidden sectors (unused) */
    uint32_t total_sect;		/* number of sectors (if sectors == 0) */

    /* The following fields are only used by FAT32 */
    uint32_t fat32_length;		/* sectors/FAT */
    uint16_t flags;		/* bit 8: fat mirroring, low 4: active fat */
    uint8_t version[2];		/* major, minor filesystem version */
    uint32_t root_cluster;		/* first cluster in root directory */
    uint16_t info_sector;		/* filesystem info sector */
    uint16_t backup_boot;		/* backup boot sector */
    uint8_t reserved2[12];		/* Unused */

    uint8_t drive_number;		/* Logical Drive Number */
    uint8_t reserved3;		/* Unused */

    uint8_t extended_sig;		/* Extended Signature (0x29) */
    uint32_t serial;		/* Serial number */
    uint8_t label[FAT_LABEL_LEN];		/* FS label */
    uint8_t fs_type[8];		/* FS Type */

    /* fill up to 512 bytes */
    uint8_t junk[422];
} __attribute__ ((packed));


struct fat_boot_sector_16 {
    uint8_t ignored[3];		/* Boot strap short or near jump */
    uint8_t system_id[8];		/* Name - can be used to special case
				   partition manager volumes */
    uint8_t sector_size[2];	/* bytes per logical sector */
    uint8_t cluster_size;		/* sectors/cluster */
    uint16_t reserved;		/* reserved sectors */
    uint8_t fats;			/* number of FATs */
    uint8_t dir_entries[2];	/* root directory entries */
    uint8_t sectors[2];		/* number of sectors */
    uint8_t media;			/* media code (unused) */
    uint16_t fat_length;		/* sectors/FAT */
    uint16_t secs_track;		/* sectors per track */
    uint16_t heads;		/* number of heads */
    uint32_t hidden;		/* hidden sectors (unused) */
    uint32_t total_sect;		/* number of sectors (if sectors == 0) */

    uint8_t drive_number;		/* Logical Drive Number */
    uint8_t reserved2;		/* Unused */

    uint8_t extended_sig;		/* Extended Signature (0x29) */
    uint32_t serial;		/* Serial number */
    uint8_t label[FAT_LABEL_LEN];		/* FS label */
    uint8_t fs_type[8];		/* FS Type */

    /* fill up to 512 bytes */
    uint8_t junk[450];
} __attribute__ ((packed));

/* uint8_t junk[446]; */
struct mbr_entry
{
    /* status / physical drive (bit 7 set: active / bootable, 
    old MBRs only accept 80h), 00h: inactive, 01h¨C7Fh: invalid) */
    uint8_t status;
    
    /* CHS address of first absolute sector in partition.
    The format is described by 3 bytes, see the next 3 rows.
    0 bytes: h[7¨C0]  head
    1 bytes: c[9-8] s[0-5] sector
    2 bytes: c[7¨C0]
    */
    uint8_t chs_begin_head;
    uint16_t chs_begin_cylinder_sector;

    /* Partition type[13] */
    uint8_t type;

    /* CHS address of last absolute sector in partition.
    The format is described by 3 bytes, see the next 3 rows.*/
    uint8_t chs_end_head;
    uint16_t chs_end_cylinder_sector;

    /* LBA of first absolute sector in the partition */
    uint32_t lba_begin;

    /* Number of sectors in partition */
    uint32_t lba_size;

}__attribute__ ((packed));

int contain_fat_label(struct disk_param * param, const char * str)
{
    /* read mbr */
    int ret;
    char buffer[1024];
    uint32_t len = sizeof(buffer);
    struct mbr_entry mbr[4];
    struct fat_boot_sector_32 * fat_ebr_32;
    struct fat_boot_sector_16 * fat_ebr_16;
    int c, h, s, i;
    char label[FAT_LABEL_LEN + 1];

    ret = read_sectors_chs(param, 0, 0, 1, 1, buffer, &len);
    if(ret != 0 || len != 512) {
        SERR("read_sectors_chs err %d", ret);
        return 0;
    }
    memcpy(mbr, buffer + 446, sizeof(mbr));
    
    for(i = 0 ; i < 4 ; i ++) {
        SDBG("mbr entry %d type %x", i, mbr[i].type);
        if((mbr[i].type == 0xB)) {
            get_chs_from_pack(mbr[i].chs_begin_cylinder_sector, &c, &s);
            h = mbr[i].chs_begin_head;
            len = sizeof(buffer);
            ret = read_sectors_chs(param, c, h, s, 1, &buffer, &len);
            SDBG("read_sectors_chs ret %d", ret);
        } else if((mbr[i].type == 0xC)) {
            SDBG("lba_begin is %d", mbr[i].lba_begin);
            len = sizeof(buffer);
            ret = read_sectors_lba(param, mbr[i].lba_begin, 1, &buffer, &len);
            SDBG("read_sectors_lba ret %d", ret);
        } else {
            continue;
        }
      
        if(ret == 0 && len == 512) {
            fat_ebr_32 = (struct fat_boot_sector_32 *) buffer;
            fat_ebr_16 = (struct fat_boot_sector_16 *) buffer;
            /* 32 label ?*/
            len = strlen(opt.skip);
            if(len > sizeof(fat_ebr_32->label)) 
                len = sizeof(fat_ebr_32->label); 
            memcpy(label, fat_ebr_32->label, sizeof(fat_ebr_32->label));
            label[sizeof(label) - 1] = 0;
            SDBG("FIND FAT32 LABEL '%s'", label);
            if(strncmp(opt.skip, label, len) == 0) {
                return 1;
            }
            /* 16 label ?*/
            len = strlen(opt.skip);
            if(len > sizeof(fat_ebr_16->label)) 
                len = sizeof(fat_ebr_16->label); 
            memcpy(label, fat_ebr_16->label, sizeof(fat_ebr_16->label));
            label[sizeof(label) - 1] = 0;
            SDBG("FIND FAT16 LABEL '%s'", label);
            if(strncmp(opt.skip, label, len) == 0) {
                return 1;
            }

        }else {
            SERR("ebr error ret %d, len %d", ret, len);
        }
    }

    return 0;
}
