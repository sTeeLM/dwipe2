#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdint.h>
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
#include "fat.h"

#include "mbr.inc"

#define WIPE_BUFFER_SIZE 8192

#define LINE_CPU	2
#define LINE_MODE	4
#define LINE_DISK_LIST 6
#define LINE_WIPE_STATUS 8
#define LINE_PROGRESS (LINE_WIPE_STATUS + 6)

struct disk_param disk_list[128];
uint32_t total_disk_cnt;
uint32_t skiped_disk_cnt;
uint32_t done_disk_cnt;
uint32_t error_disk_cnt;

static void show_header()
{
	int i;
	volatile char *pp;
	/* Make the name background red */
	for(i=0, pp=(char *)(SCREEN_ADR+1); i<SCREEN_WIDTH; i++, pp+=2) {
		*pp = 0x20;
	}
	cprint(0, 0, "                  DWiPe  v1.0.0 by sTeeL(steel.mental@gmail.com)               "); /* 80 char */

	for(i=0, pp=(char *)(SCREEN_ADR+1); i<2; i++, pp+=30) {
		*pp = 0xA4;
	}
	cprint(0, 23, "+");


}

static void clean_line(int y)
{
    int i;
    char buffer[SCREEN_WIDTH + 1];
    for(i = 0 ; i < SCREEN_WIDTH; i ++)
        buffer[i] = ' ';
    buffer[i] = 0;
    cprint(y,0, buffer);
}

static void show_sepator(int y)
{
    int i;
    char buffer[SCREEN_WIDTH + 1];
    for(i = 0 ; i < SCREEN_WIDTH; i ++)
        buffer[i] = '-';
    buffer[i] = 0;
    cprint(y,0, buffer);
}

static void show_cpu(const char * cpu, int l1, int l2, int l3, uint32_t speed_kh)
{
    char buffer[512];
    int len;

    show_sepator(LINE_CPU - 1);

    len = snprintf(buffer, sizeof(buffer), "| CPU: %s | Cache L1/L2/L3: %dK/%dK/%dK | Spd: %d.%d MHz |",
        cpu, l1, l2, l3, speed_kh/1000, (speed_kh/100)%10);
    buffer[len] = 0;
    cprint(LINE_CPU,0, buffer);

    show_sepator(LINE_CPU + 1);

}

static void show_mode(void)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "| Skip: %s | Testmode %s | Debug [%c%c%c%c] | Check: %s | Force CHS: %s |",
        opt.skip,
        opt.testmode ? "ON" : "OFF",
        opt.debug & MINIOS_LOG_ERR ? 'E': '-',
        opt.debug & MINIOS_LOG_WARN ? 'W': '-',
        opt.debug & MINIOS_LOG_INFO ? 'I': '-',
        opt.debug & MINIOS_LOG_DBG ? 'D': '-',
        opt.check ?  "ON" : "OFF",
        opt.force_chs ? "ON" : "OFF"
        );
    cprint(LINE_MODE, 0, buffer);
    show_sepator(LINE_MODE + 1);
}

static void show_disk_list(uint32_t total, uint32_t error, uint32_t skiped, uint32_t wiped)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "| Total: %02d | Error: %02d | Skiped: %02d | Wiped: %02d |", 
        total, error, skiped, wiped);
    clean_line(LINE_DISK_LIST);
    cprint(LINE_DISK_LIST, 0, buffer);
    show_sepator(LINE_DISK_LIST + 1);
}

static void show_disk_list_enum(uint8_t disk_id)
{
    char buffer[1024];
    clean_line(LINE_DISK_LIST);
    snprintf(buffer, sizeof(buffer), "| Enumerating ... %02x |", disk_id);
    cprint(LINE_DISK_LIST, 0, buffer);
    show_sepator(LINE_DISK_LIST + 1);
}

static void show_disk_status(struct disk_param *disk)
{
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "Disk %02x", disk->disk_id);
    clean_line(LINE_WIPE_STATUS);
    cprint(LINE_WIPE_STATUS, 0, buffer);

    snprintf(buffer, sizeof(buffer), "Drives %d", disk->drives);
    clean_line(LINE_WIPE_STATUS + 1);
    cprint(LINE_WIPE_STATUS + 1, 10, buffer);

    snprintf(buffer, sizeof(buffer), "Legacy C/H/S: %d/%d/%d", 
        disk->last_cylinders + 1,
        disk->last_heads + 1,
        disk->last_sectors_per_track);
    clean_line(LINE_WIPE_STATUS + 2);
    cprint(LINE_WIPE_STATUS + 2, 10, buffer);
    if(disk->has_ext) {
        snprintf(buffer, sizeof(buffer), "Extent C/H/S: %d/%d/%d",
            disk->ext.ncylinders,
            disk->ext.nheads,
            disk->ext.sectors_per_track);
        clean_line(LINE_WIPE_STATUS + 3);
        cprint(LINE_WIPE_STATUS + 3, 10, buffer);

        snprintf(buffer, sizeof(buffer), "Sector Size: %d B", disk->ext.bytes_per_sector);
        clean_line(LINE_WIPE_STATUS + 4);
        cprint(LINE_WIPE_STATUS + 4, 10, buffer);

        snprintf(buffer, sizeof(buffer), "Extent Total Sectors: %" PRIu64,disk->ext.nsectors);
        clean_line(LINE_WIPE_STATUS + 5);
        cprint(LINE_WIPE_STATUS + 5, 10, buffer);
            
    } else {
        snprintf(buffer, sizeof(buffer), "Extent Info: Not Supported");
        clean_line(LINE_WIPE_STATUS + 3);
        cprint(LINE_WIPE_STATUS + 3, 10, buffer);
        clean_line(LINE_WIPE_STATUS + 4);
        clean_line(LINE_WIPE_STATUS + 5);
    }

}

struct progress_t
{
    uint64_t total_bytes;
    uint64_t done_bytes;
    struct timeval tv_start;
    int saved_pro_width;
};

static void reset_progress(struct progress_t * progress, uint64_t total_bytes)
{
    gettimeofday(&progress->tv_start, NULL);
    progress->total_bytes = total_bytes;
    progress->done_bytes = 0;
    if(!progress->total_bytes)
        progress->total_bytes = 1;
    progress->saved_pro_width = 0;
}

/*
    |=================        |xxx.xKB/S|100%|ETA hh:mm:ss|
*/
static void update_progress(struct progress_t * progress, uint64_t write_bytes)
{
    uint32_t line_width = SCREEN_WIDTH - 30;
    uint32_t pro_width = 0;
    struct timeval tv_cur;
    uint64_t time_diff_msec, speed_bps = 0;
    uint64_t eta_total_sec = 0;
    uint32_t eta_sec = 0, eta_min = 0, eta_hour = 0;
    double speed = 0.0;
    char * speed_unit = " B/S";
    char buffer[1024];
    int len = 0, i, percent = 0;

    gettimeofday(&tv_cur, NULL);
    time_diff_msec = (tv_cur.tv_sec - progress->tv_start.tv_sec) * 1000
        + (tv_cur.tv_usec - progress->tv_start.tv_usec) / 1000;
    if(time_diff_msec == 0)
        time_diff_msec = 1;

    progress->done_bytes += write_bytes;
    
    if(progress->done_bytes > progress->total_bytes)
        progress->done_bytes = progress->total_bytes;

    pro_width = (progress->done_bytes) * line_width / progress->total_bytes;

    if(progress->saved_pro_width == pro_width) {
        return; /* don't update display too much times : blink! */
    }

    progress->saved_pro_width = pro_width;

    percent = (progress->done_bytes) * 100 / progress->total_bytes;

    speed_bps = (progress->done_bytes) * 1000 / time_diff_msec;
    if(speed_bps == 0)
        speed_bps = 1;
    eta_total_sec = (progress->total_bytes - progress->done_bytes) / speed_bps;

    eta_hour = eta_total_sec / 3600;
    eta_total_sec  = eta_total_sec % 3600; 
    eta_min  = eta_total_sec / 60;
    eta_sec  = eta_total_sec % 60; 


    SDBG("size of uint64_t is %"PRIu64, sizeof(uint64_t));

    if(speed_bps < 1024) {
        speed = speed_bps * 1000 / 1024;
        speed_unit = " B/S";
    } else if(speed_bps < 1024 * 1024){
        speed = speed_bps * 1000 / (1024 * 1024);
        speed_unit = "KB/S";
    } else if(speed_bps < 1024 * 1024 * 1024){
        speed = speed_bps * 1000 / (1024 * 1024 * 1024);
        speed_unit = "MB/S";
    } else if(speed_bps < 1024 * 1024 * 1024 * 1024){
        speed = speed_bps * 1000 / (uint64_t)(1024 * 1024 * 1024 * 1024);
        speed_unit = "GB/S";
    } else {
        speed = 999.0;
        speed_unit = "GB/S";
    }
    //|=================        |xxx.xKB/S|100%|ETA hh:mm:ss|
    len += snprintf(buffer + len, sizeof(buffer) - len, "|");
    for(i = 0 ; i < pro_width; i ++) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "%s", "=");
    }

    for(i = pro_width ; i < line_width; i ++) {
        len += snprintf(buffer + len, sizeof(buffer) - len, "%s", " ");
    }

    clean_line(LINE_PROGRESS);
    len += snprintf(buffer + len, sizeof(buffer) - len, "|%3.1f%s|%3d%%|ETA %02d:%02d:%02d|",
        speed, speed_unit, percent, eta_hour, eta_min, eta_sec);
    
    cprint(LINE_PROGRESS, 0, buffer);
    SDBG("pro_width is %d", pro_width);
    SDBG("speed_bps is %"PRIu64, speed_bps);
    SDBG("speed is %f %s", speed, speed_unit);
    SDBG("eta is %d:%d:%d", eta_hour, eta_min,eta_sec);
}

static int wipe_sectors_lba(struct disk_param * param, uint64_t offset, uint32_t nsectors, char * buffer, uint32_t * size)
{
    int ret, c, h, s;

    if(param->has_ext) {
        if(opt.testmode) {
            ret = read_sectors_lba(param, offset, nsectors, buffer, size);
            if(ret != 0) {
                SERR("read_sectors_lba error %d, id = %x nsectors = %d, size = %d", 
                    ret, 
                    param->disk_id,
                    nsectors,
                    *size);
                SERR("read_sectors_lba error %d, id = %x offset = %"PRIu64, offset);
            } 
        } else {
            ret = write_sectors_lba(param, offset, nsectors, buffer, size, opt.check);
            if(ret != 0) {
                SERR("write_sectors_lba error %d, id = %x nsectors = %d, size = %d", 
                    ret, 
                    param->disk_id,
                    nsectors,
                    *size);
                SERR("write_sectors_lba error %d, id = %x offset = %"PRIu64, offset);
            } 
        }        
    } else {
        ret = lba2chs(param, offset, &c, &h, &s);
        if(ret == 0) {
            if(opt.testmode) {
                ret = read_sectors_chs(param, c, h, s, nsectors, buffer, size);
                if(ret != 0) {
                    SERR("read_sectors_chs error %d, id = %x, CHS=%d/%d/%d nsectors = %d, size = %d", 
                        ret, c, h, s, nsectors, *size);
                }
            } else {
                ret = write_sectors_chs(param, c, h, s, nsectors, buffer, size, opt.check);
                if(ret != 0) {
                    SERR("read_sectors_chs error %d, id = %x, CHS=%d/%d/%d nsectors = %d, size = %d", 
                        ret, c, h, s, nsectors, *size);
                }
            } 
        } else {
            SERR("lba2chs error %d", ret);
            SERR("lba2chs offset %"PRIu64, offset);
        }
    }
    return ret;
}

static int is_skiped_disk(struct disk_param * param)
{
    return contain_fat_label(param, opt.skip);
}

static int wipe_disk(struct disk_param * param)
{
    struct progress_t progress;
    int ret = -1;
    uint32_t size;
    char * buffer = NULL;
    //char buffer[WIPE_BUFFER_SIZE];
    uint64_t i, total_size, total_sec, left_sec, offset_lba = 0;
    uint32_t sector_size = 512;

    SINF("try to wipe disk %x", param->disk_id);

    if(is_skiped_disk(param)) {
        SINF("skip disk %x", param->disk_id);
        return 1;
    }

    buffer = malloc(WIPE_BUFFER_SIZE);
    if(NULL == buffer) {
        SERR("oom when alloc wipe buffer %d", WIPE_BUFFER_SIZE);
        goto fail;
    }

    SDBG("wipe buffer is %p", buffer);

    if(param->has_ext && !opt.force_chs) {
        sector_size = param->ext.bytes_per_sector;
        total_sec = param->ext.nsectors;
    } else {
        total_sec = (param->last_heads + 1) * (param->last_cylinders + 1) * param->last_sectors_per_track;
    }
    total_size = total_sec * sector_size;

    reset_progress(&progress, total_size);

    for(i = 0 ; i < total_size / WIPE_BUFFER_SIZE; i ++) {
        size = WIPE_BUFFER_SIZE;
        ret =  wipe_sectors_lba(param, 
                offset_lba, 
                WIPE_BUFFER_SIZE / sector_size, 
                buffer, 
                &size);
        offset_lba += WIPE_BUFFER_SIZE / sector_size;
        SDBG("offset_lba %"PRIu64, offset_lba);
        update_progress(&progress, WIPE_BUFFER_SIZE);
        if(ret < 0) {
            goto fail;
        }
    }
    left_sec = total_sec - offset_lba;
    SDBG("left_sec %"PRIu64, offset_lba);
    if(0 != left_sec) {
        size = WIPE_BUFFER_SIZE;
        ret =  wipe_sectors_lba(param, offset_lba, left_sec, buffer, &size); 
        if(ret < 0) {
            goto fail;
        }
    }
    update_progress(&progress, left_sec * sector_size);
    SINF("wipe %x done", param->disk_id);

    if(opt.mbr && !opt.testmode) {
        memcpy(buffer, mbr_bin, mbr_bin_len);
        size = mbr_bin_len;
        ret =  wipe_sectors_lba(param, 0, 1, buffer, &size); 
        if(ret < 0) {
            goto fail;
        }
        SINF("wipe mbr done");
    }

fail:
    if(NULL != buffer) {
        free(buffer);
        buffer = NULL;
    }
    return ret;
}

void enum_disk(void)
{
    int i, index = 0;
    int ext, version;
    int ret;
    struct disk_param * param = disk_list;
    for(i = 0x80; i <= 0xff; i ++){
        //SDBG("enum disk %x", i);
        ret = get_disk_param(i, param);
        if(0 == ret) {
            show_disk_list_enum(i);
            index ++;
            ret = check_extensions_present(i, &ext, &version);
            if( ret == 0 && (ext & 0x1) ) {
                ret = get_disk_param_ext(param);
                if(ret != 0) {
                    SERR("get_disk_param_ext on %x fail %d", i, ret);
                } else {
                    SDBG("get_disk_param_ext on %x OK", i);
                }
            } else if(ret != 0){
                SERR("check_extensions_present on %x fail %d", i, ret);
            } else {
                SERR("check_extensions_present on %x ok, but not support ext, ext = %x, version = %x", i, ext, version);
            }
        } else {
            //SERR("get_disk_param on %x fail %d", i, ret);
        }
        param = &disk_list[index];
    }

    total_disk_cnt = index;

    SDBG("total disk: %d", total_disk_cnt);
}


void do_main()
{
    int i;
    int ret;

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
    
	/* Setup the display */
	display_init();

    show_header();
    show_cpu(cpu_type_str, l1_cache, l2_cache, l3_cache, speed);

    /* enum disk */
    enum_disk();

    for(i = 0 ; i < total_disk_cnt; i ++) {
        dump_disk(&disk_list[i]);
    }

    show_disk_list(total_disk_cnt, error_disk_cnt, skiped_disk_cnt, done_disk_cnt);

    for(i = 0 ; i < total_disk_cnt; i ++) {
        show_mode();
        show_disk_status(&disk_list[i]);
        ret = wipe_disk(&disk_list[i]);
        if(ret == 0) {
            done_disk_cnt ++;
        } else if(ret < 0) {
            error_disk_cnt ++;
        } else {
            skiped_disk_cnt ++;
        }
        show_disk_list(total_disk_cnt, error_disk_cnt, skiped_disk_cnt, done_disk_cnt);
    }

    reboot();
    while(1);
}

