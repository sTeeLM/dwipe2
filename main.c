#include "dwipe.h"
#include "defs.h"
#include "init.h"
#include "dwipe.h"
#include "stdint.h"
#include "disk.h"



void do_test(void)
{
    int ret;
    int ext = 0;
    int version = 0;
    int status = 0;
    int i,j,k;
    char buffer[MAX_SECTOR_BUFFER_SIZE];
    int len;

    init();
    struct disk_param param;

    /*
    hprint(line+1, 0, (ulong)ret);

    hprint(line+2, 0, (ulong)&real_buffer);
    */

    /*
    hprint(1, 0, &real_reg);
    hprint(2, 0, (ulong)&param);
    ret = get_disk_param(0x80, &param);
    hprint(3, 0, (ulong)ret);
    hprint(4, 0, (ulong)param.disk_id);
    hprint(5, 0, (ulong)param.drives);
    hprint(6, 0, (ulong)param.last_heads);
    hprint(7, 0, (ulong)param.last_cylinders);
    hprint(8, 0, (ulong)param.last_sectors_per_track);
    hprint(9, 0, (ulong)param.drive_type);
    hprint(10, 0, (ulong)param.has_ext);

    ret = get_disk_param_ext(&param);
    hprint(11, 0, (ulong)ret);
    hprint(12, 0, (ulong)param.has_ext);
    hprint(13, 0, (ulong)param.ext.size);
    hprint(14, 0, (ulong)param.ext.flags);
    hprint(15, 0, (ulong)param.ext.ncylinders);
    hprint(16, 0, (ulong)param.ext.nheads);
    hprint(17, 0, (ulong)param.ext.sectors_per_track);
    hprint2(18, 0, (ulong)param.ext.nsectors, 16);
    hprint(19, 0, (ulong)param.ext.bytes_per_sector);
    hprint(20, 0, (ulong)param.ext.edd);
    hprint(22, 0, (ulong)0);
    */

    /*
    ret = reset_disk_drive(0x80);
    hprint(1, 0, ret);
    ret = get_last_status(0x80, &status);
    hprint(2, 0, ret);
    hprint(3, 0, status);
    */

/*    
    for(i = 0 ; i < 512; i ++) {
        buffer[i]='A';
    }
    ret = get_disk_param(0x81, &param);
    for(i = 0 ; i <= param.last_cylinders; i ++)
        for(j = 0 ; j <= param.last_heads; j ++)
            for(k = 1; k <= param.last_sectors_per_track; k ++) {
        len = sizeof(buffer);
        ret = write_sectors_chs(&param, i, j, k, 1, buffer, &len, 0);
        if(ret != 0) {
            hprint(1, 0, ret);
            hprint(2, 0, i);
            hprint(3, 0, j);
            hprint(4, 0, k);
            goto fail;
        } else {
            hprint(2, 0, i);
            hprint(3, 0, j);
            hprint(4, 0, k);
        }
    }
    
    cprint(5, 0, "OK");
    goto ok;
fail:
    cprint(5, 0, "failed");
ok:
*/
/*
    ulong p;
    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    for( i = 0 ; i < param.ext.nsectors ; i ++) {
        len = sizeof(buffer);
        memset(buffer, 0, 512);
        ret = read_sectors_lba(&param, i, 1,  buffer, &len);
        if(ret != 0) {
            hprint(1, 0, ret);
            hprint(2, 0, i);
            goto fail;
        } else {
            p = buffer[3];
            p<<=8;
            p+=buffer[2];
            p<<=8;
            p+=buffer[1];
            p<<=8;
            p+=buffer[0];
            hprint(1, 0, i);
            hprint(2, 0, p);
        }
    }
    cprint(5, 0, "OK");
    goto ok;
fail:
    cprint(5, 0, "failed");
ok:
*/


    ret = get_disk_param(0x81, &param);
    ret = get_disk_param_ext(&param);
    for(i = 0 ; i < 512; i ++) {
        buffer[i]='B';
    }
    for( i = 0 ; i < param.ext.nsectors ; i ++) {
        len = sizeof(buffer);
        ret = write_sectors_lba(&param, i, 1,  buffer, &len, 0);
        if(ret != 0) {
            hprint(1, 0, ret);
            hprint(2, 0, i);
            goto fail;
        } else {
            hprint(2, 0, i);
        }
    }
    cprint(5, 0, "OK");
    goto ok;
fail:
    cprint(5, 0, "failed");
ok:


    while(1){
        
    }
}
