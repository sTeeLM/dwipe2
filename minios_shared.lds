OUTPUT_FORMAT("elf32-i386");
OUTPUT_ARCH(i386);

ENTRY(startup_32); 
SECTIONS {
	. = 0x100000;
	.text : {
		_start = .;
		*(.text)
		*(.text.*)
		*(.plt)
		_etext = . ;
	}
	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}
	.dynsym     : { *(.dynsym) }
	.dynstr     : { *(.dynstr) }
	.hash       : { *(.hash) }
	.gnu.hash   : { *(.gnu.hash) }
	.dynamic    : { *(.dynamic) }

	.rel.text    : { *(.rel.text   .rel.text.*) }
	.rel.rodata  : { *(.rel.rodata .rel.rodata.*) }
	.rel.data    : { *(.rel.data   .rel.data.*) }
	.rel.got     : { *(.rel.got    .rel.got.*) }
	.rel.plt     : { *(.rel.plt    .rel.plt.*) }

	. = ALIGN(4);
	.data : {
		 _data = .; 
		*(.data) 
		*(.data.*) 
	}
	.got : {
		*(.got.plt)
		*(.got)
		_edata = . ;
	}
	. = ALIGN(4);
	.bss : { 
		_bss = .;
		*(.dynbss)
		*(.bss) 
		*(.bss.*) 
		*(COMMON) 
		/* _end must be at least 256 byte aligned */
		. = ALIGN(256); 
		_end = .;
	}
	/DISCARD/ : { *(*) }	
}
