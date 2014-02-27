#include "defs.h"
#include "stdint.h"
#include "display.h"
#include "lib.h"
#include "io.h"
#include "disk.h"

void do_main()
{
    /* Kill Floppy Motor */
	outb(0x8, 0x3f2);  

	/* Turn on cache */
	set_cache(1);

	/* Setup the display */
	display_init();

    /* Relocate real code */
	relocate_real();

extern void do_test();
    do_test();
}

