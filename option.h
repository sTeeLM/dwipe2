#ifndef __MINIOS_OPTION_H__
#define __MINIOS_OPTION_H__

struct options_t
{
    /* skip drive if drive has a fat32 label 'skip' */
    char * skip;
    /* is test mode on? if on, only read disk not write disk */
    int testmode;
    /* 0:nothing, 1:err & warn only, 2: info & err & warn, 3: debug, info, err, warn */
    int debug;
};

extern struct options_t opt;

void dmp_opt();

void set_default_opt();

#endif
