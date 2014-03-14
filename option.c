#include <stdio.h>
#include "option.h"

struct options_t opt;

void set_default_opt()
{
    opt.skip = "NIMIME";
    opt.debug = 0;
    opt.test = 0;
    opt.check = 0;
    opt.force_chs = 0;
    opt.mbr = 0;
    opt.mode = WIPE_MODE_FAST;
}

void dmp_opt()
{
    fprintf(stderr, "opt: [skip->%s]\r\n", opt.skip == NULL ? "(none)" : opt.skip);
    fprintf(stderr, "opt: [debug->%d]\r\n", opt.debug);
    fprintf(stderr, "opt: [test->%d]\r\n", opt.test);
    fprintf(stderr, "opt: [check->%d]\r\n", opt.check);
    fprintf(stderr, "opt: [force_chs->%d]\r\n", opt.force_chs);
    fprintf(stderr, "opt: [mbr->%d]\r\n", opt.mbr);
    fprintf(stderr, "opt: [mode->%s]\r\n", opt.mode == WIPE_MODE_FAST? "FAST": (opt.mode == WIPE_MODE_DOD? "DOD":"OCD"));

}
