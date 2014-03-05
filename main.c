#include <stdio.h>
#include <inttypes.h>
#include "defs.h"
#include "stdint.h"
#include "display.h"
#include "lib.h"
#include "io.h"
#include "disk.h"
#include "serial.h"
#include "debug.h"
#include "timer.h"
#include "cmdline.h"
#include "option.h"

void wipe_disk(struct disk_param * param)
{
    int progress = 0, old_progress = 0;
    int ret;
    uint32_t size;
    char buffer[8192]; /* 4k buffer */
    uint64_t i, total_lba_r, total_lba_n, offset_lba = 0;

    int c, h, s;
    int total_sec_chs, wiped_sec_chs;

    //param->has_ext = 0;

    if(param->has_ext) {
        SDBG("wipe %x in extent mode", param->disk_id);
        
        /* how many times block write? */
        total_lba_n = param->ext.nsectors * 
            param->ext.bytes_per_sector / sizeof(buffer);
        SDBG("total_lba_n is %"PRIu64, total_lba_n);
        /* write at big 4k block */
        offset_lba = 0;
        for(i = 0 ; i < total_lba_n; i ++) {
            size = sizeof(buffer);
            ret =  read_sectors_lba(param, 
                offset_lba, 
                sizeof(buffer) / param->ext.bytes_per_sector, 
                buffer, &size);
            //SDBG("ret is %d, size is %d", ret, size);
            if(ret != 0) {
                SERR("read_sectors_lba error %d", ret);
            }
            offset_lba += sizeof(buffer) / param->ext.bytes_per_sector;
            SDBG("offset_lba is %"PRIu64, offset_lba);
            progress = offset_lba * 100 / param->ext.nsectors;
            if(old_progress != progress) {
                old_progress = progress;
                display_progress(progress, sizeof(buffer));
            }
        }
        
        /* how many sector remain ? */
        total_lba_r = param->ext.nsectors - offset_lba;
        size = sizeof(buffer);
        SDBG("total_lba_r is %"PRIu64, total_lba_r);
        ret =  read_sectors_lba(param, offset_lba, total_lba_r, buffer, &size);
        //SDBG("ret is %d, size is %d", ret, size);
        if(ret != 0) {
            SERR("read_sectors_lba error %d", ret);
        } 
        progress = 100;
        if(old_progress != progress) {
            old_progress = progress;
            display_progress(progress, total_lba_r * param->ext.bytes_per_sector);
        }
        SDBG("wipe %x in extent mode OK", param->disk_id);
    } else {
        SDBG("wipe %x in lag mode", param->disk_id);
        total_sec_chs = (param->last_cylinders + 1) * (param->last_heads + 1) * param->last_sectors_per_track;
        wiped_sec_chs = 0;
        
        for(c = 0 ; c <= param->last_cylinders; c++)
            for(h = 0 ; h <= param->last_heads; h ++)
                for(s = 1 ; s <= param->last_sectors_per_track; s ++) 
        {
            size = 512;
            ret = read_sectors_chs(param, c, h, s, 1, buffer, &size);
            if(ret != 0) {
                SERR("read_sectors_lba error %d", ret);
            } 
            wiped_sec_chs ++;
            SDBG("wiped_sec_chs is %d", wiped_sec_chs);
            progress = wiped_sec_chs * 100 / total_sec_chs;
            if(old_progress != progress) {
                old_progress = progress;
                display_progress(progress, 512);
            }
        }
        SDBG("wipe %x in lag mode OK", param->disk_id);
        progress = 100;
        if(old_progress != progress) {
            old_progress = progress;
            display_progress(progress, 512);
        }
    }
}

void do_main()
{
    uint32_t total, wiping, wiped;
    int i;

    /* Kill Floppy Motor */
	outb(0x8, 0x3f2); 

    /* init serial port */
    serial_echo_init();

    /* get and parse comand line, must before relocate_real !*/
    if(parse_cmdline() != 0) {
        reboot();
    }

    /* find cpu , get timer */
    timer_init();

	/* Turn on cache */
	set_cache(1);

    /* Relocate real code */
	relocate_real();

    /* enum disk */
    enum_disk();

    for(i = 0 ; i < disk_cnt; i ++) {
        dump_disk(&disk_list[i]);
    }
    
	/* Setup the display */
	display_init();
    display_cpu(cpu_type_str, l1_cache, l2_cache, l3_cache, speed);
    display_mode();
    display_disk_list(total, wiping, wiped);

    for(i = 0 ; i < disk_cnt; i ++) {
        display_disk_status(&disk_list[0]);
        wipe_disk(&disk_list[i]);
    }

    reboot();
}

