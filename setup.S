#define __ASSEMBLY__
#include "defs.h"

.code16
.section ".setup", "ax", @progbits
.globl start
start:

	jmp	trampoline

# This is the setup header, and it must start at %cs:2 (old 0x9020:2)

		.ascii	"HdrS"		# header signature
		.word	0x0203		# header version number (>= 0x0105)
					# or else old loadlin-1.5 will fail)
realmode_swtch:	.word	0, 0		# default_switch, SETUPSEG
start_sys_seg:	.word	0x1000
		.word	kernel_version - start
					# pointing to kernel version string
					# above section of header is compatible
					# with loadlin-1.5 (header v1.5). Don't
					# change it.

type_of_loader:	.byte	0		# = 0, old one (LILO, Loadlin,
					#      Bootlin, SYSLX, bootsect...)
					# See Documentation/i386/boot.txt for
					# assigned ids
	
# flags, unused bits must be zero (RFU) bit within loadflags
loadflags:
LOADED_HIGH	= 1			# If set, the kernel is loaded high
CAN_USE_HEAP	= 0x80			# If set, the loader also has set
					# heap_end_ptr to tell how much
					# space behind setup.S can be used for
					# heap purposes.
					# Only the loader knows what is free

		.byte	LOADED_HIGH

setup_move_size: .word  0x8000		# size to move, when setup is not
					# loaded at 0x90000. We will move setup 
					# to 0x90000 then just before jumping
					# into the kernel. However, only the
					# loader knows how much data behind
					# us also needs to be loaded.

code32_start:				# here loaders can put a different
					# start address for 32-bit code.
		.long	0x100000	# 0x100000 = default for big kernel

ramdisk_image:	.long	0		# address of loaded ramdisk image
					# Here the loader puts the 32-bit
					# address where it loaded the image.
					# This only will be read by the kernel.

ramdisk_size:	.long	0		# its size in bytes

bootsect_kludge:
		.long	0		# obsolete

heap_end_ptr:	.word	0
					# (Header version 0x0201 or later)
					# space from here (exclusive) down to
					# end of setup code can be used by setup
					# for local heap purposes.

pad1:		.word	0
cmd_line_ptr:	.long	0		# (Header version 0x0202 or later)
					# If nonzero, a 32-bit pointer
					# to the kernel command line.
					# The command line should be
					# located between the start of
					# setup and the end of low
					# memory (0xa0000), or it may
					# get overwritten before it
					# gets read.  If this field is
					# used, there is no longer
					# anything magical about the
					# 0x90000 segment; the setup
					# can be located anywhere in
					# low memory 0x10000 or higher.

ramdisk_max:	.long (-__PAGE_OFFSET-(512 << 20)-1) & 0x7fffffff
					# (Header version 0x0203 or later)
					# The highest safe address for
					# the contents of an initrd

trampoline:	call	start_of_setup

kernel_version:	.ascii	UTS_RELEASE
		.ascii	" ("
		.ascii	LINUX_COMPILE_BY
		.ascii	"@"
		.ascii	LINUX_COMPILE_HOST
		.ascii	") "
		.ascii	UTS_VERSION
		.byte	0


# ok, the read went well
# now we want to move to protected mode ...
################kernel header end###################
start_of_setup:	
	cli			# no interrupts allowed , @paddr 0x90200# 
	movb	$0x80, %al	# disable NMI for the bootup sequence
	outb	%al, $0x70

# The system will move itself to its rightful place.

	pushw	%cs
	popw	%ds
	lidt	idt_48 - start	# load idt with 0,0
	xorl	%ebx, %ebx
	pushw	%ds
	popw	%bx
	shl	$4, %ebx
	leal	gdt - start, %eax
	addl	%ebx, %eax
	movl	%eax, gdt_48 - start + 2
	lgdt	gdt_48 - start	# load gdt with whatever appropriate

	xorl	%ebx, %ebx
	pushw	%ds
	popw	%bx
	shl	$4, %ebx
	leal	cmd_line_ptr - start, %eax
	addl	%eax, %ebx      # save cmd_line_ptr to ebx

# that was painless, now we enable A20
# start from grub-a20.patch
	/*
	* try to switch gateA20 using PORT92, the "Fast A20 and Init"
	* register
	*/
	mov	$0x92, %dx
	inb	%dx, %al
	/* skip the port92 code if it's unimplemented (read returns 0xff) */
	cmpb	$0xff, %al
	jz	alt_a20_done
	
	/* set or clear bit1, the ALT_A20_GATE bit */
	movb	4(%esp), %ah
	testb	%ah, %ah
	jz	alt_a20_cont1
	orb	$2, %al
	jmp	alt_a20_cont2
alt_a20_cont1:
	and	$0xfd, %al

	/* clear the INIT_NOW bit; don't accidently reset the machine */
alt_a20_cont2:
	and	$0xfe, %al
	outb	%al, %dx

alt_a20_done:
# end from grub-a20.patch

	call    empty_8042

	movb	$0xD1, %al	# command write
	outb	%al, $0x64
	call    empty_8042

	movb	$0xDF, %al	# A20 on
	outb	%al, $0x60
	call	empty_8042

/*
 * Note that the short jump isn't strictly needed, althought there are
 * reasons why it might be a good idea. It won't hurt in any case.
 */
	movw	$0x0001, %ax	# protected mode (PE) bit
	lmsw	%ax		# This is it#
	jmp	flush_instr
flush_instr:
	movw	$KERNEL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

data32	ljmp	$KERNEL_CS, $(TSTLOAD <<4)	# jmp offset 2000 of segment 0x10 (cs)

/*
 * This routine checks that the keyboard command queue is empty
 * (after emptying the output buffers)
 *
 * No timeout is used - if this hangs there is something wrong with
 * the machine, and we probably couldn't proceed anyway.
 */
empty_8042:
	call	delay
	inb	$0x64, %al	# 8042 status port
	cmpb	$0xff, %al	# from grub-a20-patch, skip if not impl
	jz	empty_8042_ret
		
	testb	$1, %al		# output buffer?
	jz	no_output
	call	delay
	inb	$0x60, %al	# read it
	jmp	empty_8042

no_output:
	testb	$2, %al		# is input buffer full?
	jnz	empty_8042	# yes - loop
empty_8042_ret:
	ret
#
# Delay is needed after doing i/o
#
delay:
	.word	0x00eb			# jmp $+2
	ret

gdt:
	.word	0,0,0,0		# dummy

	.word	0,0,0,0		# unused

	.word	0x7FFF		# limit 128mb
	.word	0x0000		# base address=0
	.word	0x9A00		# code read/exec
	.word	0x00C0		# granularity=4096, 386

	.word	0x7FFF		# limit 128mb
	.word	0x0000		# base address=0
	.word	0x9200		# data read/write
	.word	0x00C0		# granularity=4096, 386

idt_48:
	.word	0			# idt limit=0
	.long	0			# idt base=0L

gdt_48:
	.word	0x800		# gdt limit=2048, 256 GDT entries
	.word	0
	.word	0
#	.word	512+gdt - start,0x9	# gdt base = 0X9xxxx

msg1:
	.asciz "Setup.S\r\n"

	.org	(SETUPSECS*512)

