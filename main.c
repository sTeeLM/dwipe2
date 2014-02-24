#include "dwipe.h"
#include "defs.h"
#include "init.h"
#include "dwipe.h"

volatile ulong *p = 0;

static unsigned long run_at_addr = 0xffffffff;
ulong high_test_adr = HI_TEST_ADDR;

static void __run_at(unsigned long addr)
{
	/* Copy memtest86+ code */
	memmove((void *)addr, &_start, _end - _start);
	/* Jump to the start address */
	p = (ulong *)(addr + startup_32 - _start);
	goto *p;
}

static void run_at(unsigned long addr)
{
	unsigned long start;
	unsigned long len;

	run_at_addr = addr;

	start = (unsigned long) &_start;
	len = _end - _start;
	if (	((start < addr) && ((start + len) >= addr)) ||
		((addr < start) &&  ((addr + len) >= start))) {
		/* Handle overlap by doing an extra relocation */
		if (addr + len < high_test_adr) {
			__run_at(high_test_adr);
		}
		else if (start + len < addr) {
			__run_at(LOW_TEST_ADR);
		}
	}
	__run_at(run_at_addr);
}

extern int query_pcbios(void);

void do_test(void)
{
    int line = LINE_SCROLL-2;
    int ret = 0;
    init();
	/* If we have a partial relocation finish it
    cprint(line, 0, "fuck run_at_addr is");
    hprint(line + 1, 0, (ulong)&run_at_addr);
    hprint(line + 2, 0, (ulong)run_at_addr);
    hprint(line + 3, 0, (ulong)&_start);
     */

    ret = query_pcbios();
    hprint(line, 0, (ulong)ret);
    cprint(line + 1, 0, "fuck run_at_addr is");
    hprint(line + 2, 0, (ulong)&run_at_addr);
    hprint(line + 3, 0, (ulong)run_at_addr);
    hprint(line + 4, 0, (ulong)&_start);
   

    while(1){
        check_input();
    }
}
