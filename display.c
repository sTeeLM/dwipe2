#include "display.h"
#include "io.h"

static char buf[18];

/* Except for multi-character key sequences this mapping
 * table is complete.  So it should not need to be updated
 * when new keys are searched for.  However the key handling
 * should really be turned around and only in get_key should
 * we worry about the exact keycode that was pressed.  Everywhere
 * else we should switch on the character...
 */
struct ascii_map_str {
	int ascii;
	int keycode;
};

static struct ascii_map_str ser_map[] =
/*ascii keycode     ascii  keycode*/
{
  /* Special cases come first so I can leave
   * their "normal" mapping in the table,
   * without it being activated.
   */
  {  27,   0x01}, /* ^[/ESC -> ESC  */
  { 127,   0x0e}, /*    DEL -> BS   */
  {   8,   0x0e}, /* ^H/BS  -> BS   */
  {  10,   0x1c}, /* ^L/NL  -> CR   */
  {  13,   0x1c}, /* ^M/CR  -> CR   */
  {   9,   0x0f}, /* ^I/TAB -> TAB  */
  {  19,   0x39}, /* ^S     -> SP   */
  {  17,     28}, /* ^Q     -> CR   */

  { ' ',   0x39}, /*     SP -> SP   */
  { 'a',   0x1e},
  { 'A',   0x1e},
  {   1,   0x1e}, /* ^A      -> A */
  { 'b',   0x30},
  { 'B',   0x30},
  {   2,   0x30}, /* ^B      -> B */
  { 'c',   0x2e},
  { 'C',   0x2e},
  {   3,   0x2e}, /* ^C      -> C */
  { 'd',   0x20},
  { 'D',   0x20},
  {   4,   0x20}, /* ^D      -> D */
  { 'e',   0x12},
  { 'E',   0x12},
  {   5,   0x12}, /* ^E      -> E */
  { 'f',   0x21},
  { 'F',   0x21},
  {   6,   0x21}, /* ^F      -> F */
  { 'g',   0x22},
  { 'G',   0x22},
  {   7,   0x22}, /* ^G      -> G */
  { 'h',   0x23},
  { 'H',   0x23},
  {   8,   0x23}, /* ^H      -> H */
  { 'i',   0x17},
  { 'I',   0x17},
  {   9,   0x17}, /* ^I      -> I */
  { 'j',   0x24},
  { 'J',   0x24},
  {  10,   0x24}, /* ^J      -> J */
  { 'k',   0x25},
  { 'K',   0x25},
  {  11,   0x25}, /* ^K      -> K */
  { 'l',   0x26},
  { 'L',   0x26},
  {  12,   0x26}, /* ^L      -> L */
  { 'm',   0x32},
  { 'M',   0x32},
  {  13,   0x32}, /* ^M      -> M */
  { 'n',   0x31},
  { 'N',   0x31},
  {  14,   0x31}, /* ^N      -> N */
  { 'o',   0x18},
  { 'O',   0x18},
  {  15,   0x18}, /* ^O      -> O */
  { 'p',   0x19},
  { 'P',   0x19},
  {  16,   0x19}, /* ^P      -> P */
  { 'q',   0x10},
  { 'Q',   0x10},
  {  17,   0x10}, /* ^Q      -> Q */
  { 'r',   0x13},
  { 'R',   0x13},
  {  18,   0x13}, /* ^R      -> R */
  { 's',   0x1f},
  { 'S',   0x1f},
  {  19,   0x1f}, /* ^S      -> S */
  { 't',   0x14},
  { 'T',   0x14},
  {  20,   0x14}, /* ^T      -> T */
  { 'u',   0x16},
  { 'U',   0x16},
  {  21,   0x16}, /* ^U      -> U */
  { 'v',   0x2f},
  { 'V',   0x2f},
  {  22,   0x2f}, /* ^V      -> V */
  { 'w',   0x11},
  { 'W',   0x11},
  {  23,   0x11}, /* ^W      -> W */
  { 'x',   0x2d},
  { 'X',   0x2d},
  {  24,   0x2d}, /* ^X      -> X */
  { 'y',   0x15},
  { 'Y',   0x15},
  {  25,   0x15}, /* ^Y      -> Y */
  { 'z',   0x2c},
  { 'Z',   0x2c},
  {  26,   0x2c}, /* ^Z      -> Z */
  { '-',   0x0c},
  { '_',   0x0c},
  {  31,   0x0c}, /* ^_      -> _ */
  { '=',   0x0c},
  { '+',   0x0c},
  { '[',   0x1a},
  { '{',   0x1a},
  {  27,   0x1a}, /* ^[      -> [ */
  { ']',   0x1b},
  { '}',   0x1b},
  {  29,   0x1b}, /* ^]      -> ] */
  { ';',   0x27},
  { ':',   0x27},
  { '\'',  0x28},
  { '"',   0x28},
  { '`',   0x29},
  { '~',   0x29},
  { '\\',  0x2b},
  { '|',   0x2b},
  {  28,   0x2b}, /* ^\      -> \ */
  { ',',   0x33},
  { '<',   0x33},
  { '.',   0x34},
  { '>',   0x34},
  { '/',   0x35},
  { '?',   0x35},
  { '1',   0x02},
  { '!',   0x02},
  { '2',   0x03},
  { '@',   0x03},
  { '3',   0x04},
  { '#',   0x04},
  { '4',   0x05},
  { '$',   0x05},
  { '5',   0x06},
  { '%',   0x06},
  { '6',   0x07},
  { '^',   0x07},
  {  30,   0x07}, /* ^^      -> 6 */
  { '7',   0x08},
  { '&',   0x08},
  { '8',   0x09},
  { '*',   0x09},
  { '9',   0x0a},
  { '(',   0x0a},
  { '0',   0x0b},
  { ')',   0x0b},
  {   0,      0}
};

/*
 * Given an ascii character, return the keycode
 *
 * Uses ser_map definition above.
 *
 * It would be more efficient to use an array of 255 characters
 * and directly index into it.
 */
static int ascii_to_keycode (int in)
{
	struct ascii_map_str *p;
	for (p = ser_map; p->ascii; p++) {
		if (in ==p->ascii)
			return p->keycode;
	}
	return 0;
}


/*
 * Print characters on screen
 */
void cprint(int y, int x, const char *text)
{
	register int i;
	char *dptr;

	dptr = (char *)(SCREEN_ADR + (160*y) + (2*x));
	for (i=0; text[i]; i++) {
		*dptr = text[i];
		dptr += 2;
	}
}

/*
 * Print a decimal number on screen
 */
void dprint(int y, int x, uint32_t val, int len, int right)
{
	uint32_t j, k;
	int i, flag=0;

	if (val > 999999999 || len > 9) {
		return;
	}
	for(i=0, j=1; i<len-1; i++) {
		j *= 10;
	}
	if (!right) {
		for (i=0; j>0; j/=10) {
			k = val/j;
			if (k > 9) {
				j *= 100;
				continue;
			}
			if (flag || k || j == 1) {
				buf[i++] = k + '0';
				flag++;
			} else {
				buf[i++] = ' ';
			}
			val -= k * j;
		}
	} else {
		for(i=0; i<len; j/=10) {
			if (j) {
				k = val/j;
					if (k > 9) {
					j *= 100;
					len++;
					continue;
				}
				if (k == 0 && flag == 0) {
					continue;
				}
				buf[i++] = k + '0';
				val -= k * j;
			} else {
				if (flag == 0 &&  i < len-1) {
					buf[i++] = '0';
				} else {
					buf[i++] = ' ';
				}
			}
			flag++;
		}
	}
	buf[i] = 0;
	cprint(y,x,buf);
}

/*
 * Print a hex number on screen
 */
void hprint(int y, int x, unsigned long val)
{
	return hprint2(y, x, val, 8);
}

/*
 * Print a hex number on screen at least digits long
 */
void hprint2(int y,int x, unsigned long val, int digits)
{
        unsigned long j;
        int i, idx, flag = 0;

        for (i=0, idx=0; i<8; i++) {
                j = val >> (28 - (4 * i));
                j &= 0xf;
                if (j < 10) {
                        if (flag || j || i == 7) {
                                buf[idx++] = j + '0';
                                flag++;
                        } else {
                                buf[idx++] = '0';
                        }
                } else {
                        buf[idx++] = j + 'a' - 10;
                        flag++;
                }
        }
        if (digits > 8) {
                digits = 8;
        }
        if (flag > digits) {
                digits = flag;
        }
        buf[idx] = 0;
        cprint(y,x,buf + (idx - digits));
}

int get_key() 
{
	int c;

	c = inb(0x64);
	if ((c & 1) == 0) {
		return(0);
	}
	c = inb(0x60);
	return((c));
}

void check_input(void)
{
	unsigned char c;

	if ((c = get_key())) {
		switch(c & 0x7f) {
		case 1:
			/* "ESC" key was pressed, bail out.  */
			cprint(LINE_RANGE, COL_MID+23, "Halting... ");

			/* tell the BIOS to do a warm start */
			*((unsigned short *)0x472) = 0x1234;
			outb(0xfe,0x64);
			break;
		case 46:
			/* c - Configure */
			//get_config();
			break;
		case 28:
			/* CR - clear scroll lock */
			//slock = 0;
			//footer();
			break;
		case 57:
			/* SP - set scroll lock */
			//slock = 1;
			//footer();
			break;
		case 0x26:
			/* ^L/L - redraw the display */
			//tty_print_screen();
			break;
		}
	}
}

void display_init(void)
{
	int i;
	volatile char *pp;

	/* Clear screen & set background to blue */
	for(i=0, pp=(char *)(SCREEN_ADR); i<80*24; i++) {
		*pp++ = ' ';
		*pp++ = 0x17;
	}

	/* Make the name background red */
	for(i=0, pp=(char *)(SCREEN_ADR+1); i<TITLE_WIDTH; i++, pp+=2) {
		*pp = 0x20;
	}
	cprint(0, 0, "      DWiPe  v1.0.0      ");

	for(i=0, pp=(char *)(SCREEN_ADR+1); i<2; i++, pp+=30) {
		*pp = 0xA4;
	}
	cprint(0, 15, "+");

	/* Do reverse video for the bottom display line */
	for(i=0, pp=(char *)(SCREEN_ADR+1+(24 * 160)); i<80; i++, pp+=2) {
		*pp = 0x71;
	}

}
