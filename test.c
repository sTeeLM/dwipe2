#include <sys/unistd.h>
#include <stdio.h>
#include "defs.h"
#include "stdint.h"
#include "display.h"
#include "lib.h"
#include "io.h"
#include "disk.h"

void test1()
{
    int ret, status;
    struct disk_param param;
    display_init();
    cprint(2, 0, "TESTING get_disk_param, get_disk_param_ext, reset_disk_drive, get_last_status");

    ret = get_disk_param(0x81, &param);
    hprint(3, 0, (uint32_t)ret);
    hprint(4, 0, (uint32_t)param.disk_id);
    hprint(5, 0, (uint32_t)param.drives);
    hprint(6, 0, (uint32_t)param.last_heads);
    hprint(7, 0, (uint32_t)param.last_cylinders);
    hprint(8, 0, (uint32_t)param.last_sectors_per_track);
    hprint(9, 0, (uint32_t)param.drive_type);
    hprint(10, 0, (uint32_t)param.has_ext);

    ret = get_disk_param_ext(&param);
    hprint(12, 0, (uint32_t)ret);
    hprint(13, 0, (uint32_t)param.has_ext);
    hprint(14, 0, (uint32_t)param.ext.size);
    hprint(15, 0, (uint32_t)param.ext.flags);
    hprint(16, 0, (uint32_t)param.ext.ncylinders);
    hprint(17, 0, (uint32_t)param.ext.nheads);
    hprint(18, 0, (uint32_t)param.ext.sectors_per_track);
    hprint2(19, 0, (uint32_t)param.ext.nsectors, 16);
    hprint(20, 0, (uint32_t)param.ext.bytes_per_sector);
    hprint(21, 0, (uint32_t)param.ext.edd);
    sleep(5);

    display_init();
    cprint(2, 0, "TESTING reset_disk_drive");

    ret = reset_disk_drive(0x81);
    hprint(3, 0, ret);
    ret = get_last_status(0x81, &status);
    hprint(4, 0, ret);
    hprint(5, 0, status);
    sleep(5);
}

void test2()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read chs, 1 sector ");
    ret = get_disk_param(0x81, &param);
    len = sizeof(buffer);
    ret = read_sectors_chs(&param, 0, 0, 1, 1, buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test3()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read chs, 8 sector ");
    ret = get_disk_param(0x81, &param);
    len = sizeof(buffer);
    ret = read_sectors_chs(&param, 0, 0, 1, 8, buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[3584]);
    hprint(5, 0, buffer[3585]);
    hprint(6, 0, buffer[3586]);
    hprint(7, 0, buffer[3587]);
    hprint(8, 0, len);
    sleep(5);
}

void test4()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    memset(buffer, 'B', sizeof(buffer));
    cprint(2, 0, "TESTING raw write chs, 1 sector ");
    ret = get_disk_param(0x81, &param);
    len = 512;
    ret = write_sectors_chs(&param, 0, 0, 1, 1, buffer, &len, 0);
    hprint(3, 0, ret);
    hprint(4, 0, len);
    memset(buffer, 0, sizeof(buffer));
    len = sizeof(buffer);
    ret = read_sectors_chs(&param, 0, 0, 1, 1, buffer, &len);
    hprint(5, 0, ret);
    hprint(6, 0, buffer[0]);
    hprint(7, 0, buffer[1]);
    hprint(8, 0, buffer[2]);
    hprint(9, 0, buffer[3]); 
    hprint(10, 0, len);
    sleep(5);
}

void test5()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    ret = get_disk_param(0x81, &param);
    memset(buffer, 'C', sizeof(buffer));
    cprint(2, 0, "TESTING raw write chs, 8 sector ");
    len = sizeof(buffer);
    ret = write_sectors_chs(&param, 0, 0, 1, 8, buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, len);
    memset(buffer, 0, sizeof(buffer));
    len = sizeof(buffer);
    ret = read_sectors_chs(&param, 0, 0, 1, 8, buffer, &len);
    hprint(5, 0, ret);
    hprint(6, 0, buffer[3584]);
    hprint(7, 0, buffer[3585]);
    hprint(8, 0, buffer[3586]);
    hprint(9, 0, buffer[3587]);
    hprint(10, 0, len);
    sleep(5);
}

void test6()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;
    int i,j,k;  
    
    display_init();
    cprint(2, 0, "TESTING raw read chs all");
    ret = get_disk_param(0x81, &param);
    for(i = 0 ; i <= param.last_cylinders; i ++)
        for(j = 0 ; j <= param.last_heads; j ++)
            for(k = 1; k <= param.last_sectors_per_track; k ++) {
        len = 512;
        ret = read_sectors_chs(&param, i, j, k, 1, buffer, &len);
        if(ret != 0) {
            hprint(3, 0, ret);
            hprint(4, 0, i);
            hprint(5, 0, j);
            hprint(6, 0, k);
            cprint(7, 0, "FAIL");
            goto fail0;
        } else {
            hprint(4, 0, i);
            hprint(5, 0, j);
            hprint(6, 0, k);
        }
    }
    cprint(7, 0, "OK");
fail0:
    sleep(5);
}

void test7()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;
    int i,j,k;  
    
    display_init();
    cprint(2, 0, "TESTING raw write chs all");
    ret = get_disk_param(0x81, &param);
    memset(buffer, 'C', sizeof(buffer));
    for(i = 0 ; i <= param.last_cylinders; i ++)
        for(j = 0 ; j <= param.last_heads; j ++)
            for(k = 1; k <= param.last_sectors_per_track; k ++) {
        len = 512;
        ret = write_sectors_chs(&param, i, j, k, 1, buffer, &len, 1);
        if(ret != 0) {
            hprint(3, 0, ret);
            hprint(4, 0, i);
            hprint(5, 0, j);
            hprint(6, 0, k);
            cprint(7, 0, "FAIL");
            goto fail0;
        } else {
            hprint(4, 0, i);
            hprint(5, 0, j);
            hprint(6, 0, k);
        }
    }
    cprint(7, 0, "OK");
fail0:
    sleep(5);
}

void test8()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write chs, last 1 sector, should ok ");
    ret = get_disk_param(0x81, &param);
    memset(buffer, 'X', sizeof(buffer));
    len = sizeof(buffer);
    ret = write_sectors_chs(&param, param.last_cylinders, param.last_heads, param.last_sectors_per_track, 1, buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test9()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write chs, last 8 sector, 7 sector fail");
    ret = get_disk_param(0x81, &param);
    memset(buffer, 'Y', sizeof(buffer));
    len = sizeof(buffer);
    ret = write_sectors_chs(&param, param.last_cylinders, param.last_heads, param.last_sectors_per_track, 8, buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len); /* not ok ? should be 512 ? */
    sleep(5);
}

void test10()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read chs, last 8 sector, 7 sector fail");
    ret = get_disk_param(0x81, &param);
    memset(buffer, 'Y', sizeof(buffer));
    len = sizeof(buffer);
    ret = read_sectors_chs(&param, param.last_cylinders, param.last_heads, param.last_sectors_per_track, 8, buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len); /* not ok ? should be 512 ? */
    sleep(5);
}

void test11()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read lba, 1 sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    len = sizeof(buffer);
    ret = read_sectors_lba(&param, 0, 1, buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test12()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read lba, 8 sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    len = sizeof(buffer);
    ret = read_sectors_lba(&param, 0, 8, buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[3584]);
    hprint(5, 0, buffer[3585]);
    hprint(6, 0, buffer[3586]);
    hprint(7, 0, buffer[3587]);
    hprint(8, 0, len);
    sleep(5);
}

void test13()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read lba, last sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    len = sizeof(buffer);
    ret = read_sectors_lba(&param, param.ext.nsectors - 1,1 , buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test14()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw read lba, last sector, 2 sector, 1 should fail ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    len = sizeof(buffer);
    ret = read_sectors_lba(&param, param.ext.nsectors - 1, 2 , buffer, &len);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len); /* OK , is 512*/
    sleep(5);
}


void test15()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write lba, 1 sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    memset(buffer, 'F', sizeof(buffer));
    len = sizeof(buffer);
    ret = write_sectors_lba(&param, 0, 1, buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test16()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write lba, 8 sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    len = sizeof(buffer);
    memset(buffer, 'K', sizeof(buffer));
    ret = write_sectors_lba(&param, 0, 8, buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[3584]);
    hprint(5, 0, buffer[3585]);
    hprint(6, 0, buffer[3586]);
    hprint(7, 0, buffer[3587]);
    hprint(8, 0, len);
    sleep(5);
}

void test17()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write lba, last sector ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    memset(buffer, 'P', sizeof(buffer));
    len = sizeof(buffer);
    ret = write_sectors_lba(&param, param.ext.nsectors - 1,1 , buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len);
    sleep(5);
}

void test18()
{
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    struct disk_param param;
    uint32_t len; int ret;

    display_init();
    cprint(2, 0, "TESTING raw write lba, last sector, 10 sector, 9 should fail ");
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    memset(buffer, 'M', sizeof(buffer));
    len = sizeof(buffer);
    ret = write_sectors_lba(&param, param.ext.nsectors - 1, 10 , buffer, &len, 1);
    hprint(3, 0, ret);
    hprint(4, 0, buffer[0]);
    hprint(5, 0, buffer[1]);
    hprint(6, 0, buffer[2]);
    hprint(7, 0, buffer[3]);
    hprint(8, 0, len); /* fail , shoudld 512 ?*/
    sleep(5); 
}

void do_test()
{
    /*
    test1();
    test2();
    test3();
    test4();
    test5();
    */

 //   test14();
    test18();

    FILE * f = fopen("/dev/zero", "r");

    while(1);
}
