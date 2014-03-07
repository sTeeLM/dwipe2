#ifndef __MINIOS_FAT_H__
#define __MINIOS_FAT_H__

#include "disk.h"

int contain_fat_label(struct disk_param * param, const char * str);

#endif
