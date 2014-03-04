#include "serial.h"

#define SERIAL_TTY 0
#define SERIAL_BAUD_RATE 9600
#define SERIAL_CONSOLE_DEFAULT 0

int slock = 0, lsr = 0;
short serial_cons = SERIAL_CONSOLE_DEFAULT;

#if SERIAL_TTY != 0 && SERIAL_TTY != 1
#error Bad SERIAL_TTY. Only ttyS0 and ttyS1 are supported.
#endif
short serial_tty = SERIAL_TTY;
const short serial_base_ports[] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};

#if ((115200%SERIAL_BAUD_RATE) != 0)
#error Bad default baud rate
#endif
int serial_baud_rate = SERIAL_BAUD_RATE;
unsigned char serial_parity = 0;
unsigned char serial_bits = 8;

void serial_echo_init(void)
{
	int comstat, hi, lo, serial_div;
	unsigned char lcr;

	/* read the Divisor Latch */
	comstat = serial_echo_inb(UART_LCR);
	serial_echo_outb(comstat | UART_LCR_DLAB, UART_LCR);
	hi = serial_echo_inb(UART_DLM);
	lo = serial_echo_inb(UART_DLL);
	serial_echo_outb(comstat, UART_LCR);

	/* now do hardwired init */
	lcr = serial_parity | (serial_bits - 5);
	serial_echo_outb(lcr, UART_LCR); /* No parity, 8 data bits, 1 stop */
	serial_div = 115200 / serial_baud_rate;
	serial_echo_outb(0x80|lcr, UART_LCR); /* Access divisor latch */
	serial_echo_outb(serial_div & 0xff, UART_DLL);  /* baud rate divisor */
	serial_echo_outb((serial_div >> 8) & 0xff, UART_DLM);
	serial_echo_outb(lcr, UART_LCR); /* Done with divisor */


	/* Prior to disabling interrupts, read the LSR and RBR
	 * registers */
	comstat = serial_echo_inb(UART_LSR); /* COM? LSR */
	comstat = serial_echo_inb(UART_RX);	/* COM? RBR */
	serial_echo_outb(0x00, UART_IER); /* Disable all interrupts */

	return;
}

void serial_echo_print(const char *p)
{
	if (!serial_cons) {
		return;
	}
	/* Now, do each character */
	while (*p) {
		WAIT_FOR_XMITR;

		/* Send the character out. */
		serial_echo_outb(*p, UART_TX);
		if(*p==10) {
			WAIT_FOR_XMITR;
			serial_echo_outb(13, UART_TX);
		}
		p++;
	}
}

void serial_echo(int ch)
{
	int lsr;
	WAIT_FOR_XMITR;
	serial_echo_outb(ch, UART_TX);
}
