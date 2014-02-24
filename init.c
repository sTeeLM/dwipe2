#include "dwipe.h"
#include "init.h"
#include "pci.h"
#include "io.h"
#include "serial.h"

struct cpu_ident cpu_id;

static void display_init(void)
{
	int i;
	volatile char *pp;

	serial_echo_init();
	serial_echo_print("\x1B[LINE_SCROLL;24r"); /* Set scroll area row 7-23 */
	serial_echo_print("\x1B[H\x1B[2J");   /* Clear Screen */
	serial_echo_print("\x1B[37m\x1B[44m");
	serial_echo_print("\x1B[0m");
	serial_echo_print("\x1B[37m\x1B[44m");

	/* Clear screen & set background to blue */
	for(i=0, pp=(char *)(SCREEN_ADR); i<80*24; i++) {
		*pp++ = ' ';
		*pp++ = 0x17;
	}

	/* Make the name background red */
	for(i=0, pp=(char *)(SCREEN_ADR+1); i<TITLE_WIDTH; i++, pp+=2) {
		*pp = 0x20;
	}
	cprint(0, 0, "      Memtest86  v4.20      ");

	for(i=0, pp=(char *)(SCREEN_ADR+1); i<2; i++, pp+=30) {
		*pp = 0xA4;
	}
	cprint(0, 15, "+");

	/* Do reverse video for the bottom display line */
	for(i=0, pp=(char *)(SCREEN_ADR+1+(24 * 160)); i<80; i++, pp+=2) {
		*pp = 0x71;
	}

	serial_echo_print("\x1B[0m");
}

void init(void)
{
	outb(0x8, 0x3f2);  /* Kill Floppy Motor */

	/* Turn on cache */
	set_cache(1);

	/* Setup the display */
	display_init();

	/* setup pci */
	pci_init();
}