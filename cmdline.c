#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "cmdline.h"
#include "option.h"

#define MAX_CMDLINE_SIZE 1024
#define MAX_CMDLINE_SLOT 64

extern char * cmd_line_ptr; /* in head.S*/

static char cmd_line[MAX_CMDLINE_SIZE];
static uint32_t cmd_slot_cnt;

struct cmd_slot_t
{
    char * key;
    char * val;
};

static struct cmd_slot_t cmd_slot[MAX_CMDLINE_SLOT];

static int is_cmd_exist(const char * key)
{
    int i;
    for(i = 0 ; i < cmd_slot_cnt ; i ++) {
        if(!strcmp(cmd_slot[i].key, key)) {
            return 1;
        }
    }
    return 0;
}

static char * get_cmd_item(const char * key)
{
    int i;
    for(i = 0 ; i < cmd_slot_cnt ; i ++) {
        if(!strcmp(cmd_slot[i].key, key)) {
            return cmd_slot[i].val;
        }
    }
    return NULL;
}

int parse_cmdline()
{
    int i, is_val, index = 0;
    char * str1, *str2, *saveptr1, *saveptr2, *token, *subtoken;

    fprintf(stderr, "==================welcom to minios!=================\r\n", cmd_line_ptr);

    fprintf(stderr, "cmd is %s\r\n", cmd_line_ptr);
    strncpy(cmd_line, cmd_line_ptr, sizeof(cmd_line));
    cmd_line[sizeof(cmd_line) - 1] = 0;

    memset(cmd_slot, 0, sizeof(cmd_slot));

    for (i = 1, str1 = cmd_line; ; i++, str1 = NULL) {
        token = strtok_r(str1, ", ", &saveptr1);
        if (token == NULL)
            break;
        is_val = 0;
        for (str2 = token; ; str2 = NULL) {
            subtoken = strtok_r(str2, "=", &saveptr2);
            if (subtoken == NULL)
                break;
            if(!is_val) {
                cmd_slot[index].key = subtoken;
                is_val = 1;
            } else {
                cmd_slot[index].val = subtoken;
                is_val = 0;
            }
        }
        index ++;
        if(index > MAX_CMDLINE_SLOT)
            break;
    }
    cmd_slot_cnt = index;
    fprintf(stderr, "cmd item cnt %d\r\n", cmd_slot_cnt);
    for(i = 0 ; i < cmd_slot_cnt; i ++) {
        fprintf(stderr, "cmd: [%s->%s]\r\n", cmd_slot[i].key, cmd_slot[i].val == NULL ? "(none)" : cmd_slot[i].val);
    }

    set_default_opt();
    fprintf(stderr, "before parse options\r\n");
    dmp_opt();
    
    if(is_cmd_exist("testmode")) {
        opt.testmode = 1;
    }

    if(is_cmd_exist("check")) {
        opt.check = 1;
    }

    if(is_cmd_exist("force_chs")) {
        opt.force_chs = 1;
    }

    if(is_cmd_exist("mbr")) {
        opt.mbr = 1;
    }

    if(is_cmd_exist("debug") && get_cmd_item("debug") != NULL) {
        opt.debug = atoi(get_cmd_item("debug"));
        if(opt.debug < 0) opt.debug = 0;
        if(opt.debug > 0xf) opt.debug = 0xf;
    }

    if(is_cmd_exist("skip") && get_cmd_item("skip") != NULL) {
        opt.skip = get_cmd_item("skip");
    }
    
    fprintf(stderr, "after parse options\r\n");
    dmp_opt();
    return 0;
}

