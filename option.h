#ifndef __MINIOS_OPTION_H__
#define __MINIOS_OPTION_H__

struct options_t
{
    /* skip drive if drive has a fat32 label 'skip' */
    char * skip;
    /* is test mode on? if on, only read disk not write disk */
    int testmode;
    /* err warn info debug
    0x1(0001) err
    0x3(0011) err & warn
    0x7(0111) err & warn & info
    0xf(1111) err & warn & info & debug
    */
    int debug;
    /* 0: write no check, 1: write & check */
    int check;
    /* 1: force chs mode, 0: try use lba mode */
    int force_chs;
    /* 1: write a mbr, 0: do nothing */
    int mbr;
};

extern struct options_t opt;

void dmp_opt();

void set_default_opt();

#endif
