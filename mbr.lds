OUTPUT_FORMAT("binary")
OUTPUT_ARCH("i386")

ENTRY(_boot);
SECTIONS {
	. = 0x7c00;
	.bootsect : { *(.bootsect) }
}
