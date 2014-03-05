#include <stdio.h>
#include "option.h"

struct options_t opt;

void set_default_opt()
{
    opt.skip = "NIMIME";
    opt.debug = 0;
    opt.testmode = 0;
}

void dmp_opt()
{
    fprintf(stderr, "opt: [skip->%s]\r\n", opt.skip == NULL ? "(none)" : opt.skip);
    fprintf(stderr, "opt: [debug->%d]\r\n", opt.debug);
    fprintf(stderr, "opt: [testmode->%d]\r\n", opt.testmode);
}
