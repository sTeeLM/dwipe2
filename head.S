.text
#define __ASSEMBLY__
#include "defs.h"

#define APM_OFF

/*        MEMORY LAYOUT

	   boot strap
        |  Protected-mode kernel |
100000  +------------------------+
	|  I/O memory hole	 |
0A0000	+------------------------+
	|  Reserved for BIOS	 |	Leave as much as possible unused
	~                        ~
	|  Command line		 |	(Can also be below the X+10000 mark)
X+10000	+------------------------+
	|  Stack/heap		 |	For use by the kernel real-mode code.
X+08000	+------------------------+	
	|  Kernel setup		 |	The kernel real-mode code.
	|  Kernel boot sector	 |	The kernel legacy boot sector.
X       +------------------------+
	|  Boot loader		 |	<- Boot sector entry point 0000:7C00
001000	+------------------------+
	|  Reserved for MBR/BIOS |
000800	+------------------------+
	|  Typically used by MBR |
000600	+------------------------+ 
	|  BIOS use only	 |
000000	+------------------------+

	   relocate real code
        |  Protected-mode kernel |
100000  +------------------------+
	|  I/O memory hole	 |
0A0000	+------------------------+
	|  Reserved for BIOS	 |	Leave as much as possible unused
	~                        ~
	|  Command line		 |	(Can also be below the X+10000 mark)
020000	+------------------------+
	|  real stack		 |	For use by the real-mode code.
Y	+------------------------+	
	|  real data		 |	
	|  real code     	 |	relocated real code
010000  +------------------------+
	|  Boot loader		 |	<- Boot sector entry point 0000:7C00
001000	+------------------------+
	|  Reserved for MBR/BIOS |
000800	+------------------------+
	|  Typically used by MBR |
000600	+------------------------+ 
	|  BIOS use only	 |
000000	+------------------------+
*/

/*
 * References to members of the boot_cpu_data structure.
 */
#define CPU_PARAMS	cpu_id
#define X86		0
#define X86_MODEL	1
#define X86_MASK	2
#define X86_CPUID	4
#define X86_CAPABILITY	8
#define X86_VENDOR_ID	12
#define X86_CACHE	24
#define X86_PWRCAP 40
#define X86_EXT	44
#define X86_FFL	48
#define X86_DCACHE0_EAX	52
#define X86_DCACHE0_EBX	56
#define X86_DCACHE0_ECX	60
#define X86_DCACHE0_EDX	64
#define X86_DCACHE1_EAX	68
#define X86_DCACHE1_EBX	72
#define X86_DCACHE1_ECX	76
#define X86_DCACHE1_EDX	80
#define X86_DCACHE2_EAX	84
#define X86_DCACHE2_EBX	88
#define X86_DCACHE2_ECX	92
#define X86_DCACHE2_EDX	96
#define X86_DCACHE3_EAX	100
#define X86_DCACHE3_EBX	104
#define X86_DCACHE3_ECX	108
#define X86_DCACHE3_EDX	112

	.code32
	.globl startup_32
/* ebx point to cmd_line_ptr */
startup_32:
	cld
	cli

	/* Pick the appropriate stack address */
	leal	stack_top, %esp

	/* Reload all of the segment registers */
	leal	gdt, %eax
	movl	%eax, 2 + gdt_descr
	lgdt	gdt_descr
	leal	flush, %eax
	pushl	$KERNEL_CS
	pushl	%eax
	lret
flush:	movl	$KERNEL_DS, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	movl	(%ebx), %eax
	movl	%eax, cmd_line_ptr
/*
 *  Zero BSS
 */
	cmpl	$1, zerobss
	jnz	zerobss_done
	xorl	%eax, %eax
	leal	_bss, %edi
	leal	_end, %ecx
	subl	%edi, %ecx
1:	movl	%eax, (%edi)
	addl	$4, %edi
	subl	$4, %ecx
	jnz	1b
	movl	$0, zerobss
zerobss_done:

/*
 * fix up gdt for prot->real trans, relocate real code
 */
	/* Compute the reloc address */
	leal	startup_32, %esi

	/* Compute the gdt fixup */
	movl	%esi, %eax
	shll	$16, %eax

	movl	%esi, %ecx
	shrl	$16, %ecx
	andl	$0xff, %ecx

	movl	%esi, %edx
	andl	$0xff000000, %edx
	orl	%edx, %ecx

	/* Fixup the gdt */
	andl	$0x0000ffff, REAL_CS + 0 + gdt
	orl	%eax,        REAL_CS + 0 + gdt
	andl	$0x00ffff00, REAL_CS + 4 + gdt
	orl	%ecx,        REAL_CS + 4 + gdt
	andl	$0x0000ffff, REAL_DS + 0 + gdt
	orl	%eax,        REAL_DS + 0 + gdt
	andl	$0x00ffff00, REAL_DS + 4 + gdt
	orl	%ecx,        REAL_DS + 4 + gdt
/*
 * Clear the video display
 */
	cmpl	$1, clear_display
	jnz	clear_display_done
	movw	$0x0720, %ax
	movl	$0xb8000, %edi
	movl	$0xc0000, %ecx
1:	movw	%ax, (%edi)
	addl	$2, %edi
	cmpl	%ecx, %edi
	jnz	1b
	movl	$0, clear_display
clear_display_done:


/*
 * Setup and exception handler
 */
	leal	idt, %edi

	leal	vec0, %edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx, %ax	/* selector = 0x0010 = cs */
	movw	$0x8E00, %dx	/* interrupt gate - dpl=0, present */
	movl	%eax, (%edi)
	movl	%edx, 4(%edi)
	addl	$8, %edi

	leal	vec1,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec2,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec3,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec4,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec5,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec6,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec7,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec8,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec9,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec10,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec11,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec12,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec13,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec14,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec15,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec16,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec17,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec18,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	leal	vec19,%edx
	movl	$(KERNEL_CS << 16),%eax
	movw	%dx,%ax		   /* selector = 0x0010 = cs */
	movw	$0x8E00,%dx	   /* interrupt gate - dpl=0, present */
	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi

	/* Now that it is initialized load the interrupt descriptor table */
	leal	idt, %eax
	movl	%eax, 2 + idt_descr
	lidt	idt_descr

/* Find out the CPU type */

	leal	cpu_id, %esi
	movl	%ebx, %edi

	movl	$-1, X86_CPUID(%esi)	#  -1 for no CPUID initially

/* check if it is 486 or 386. */

	movl	$3, X86(%esi)	# at least 386
	pushfl			# push EFLAGS
	popl	%eax		# get EFLAGS
	movl	%eax, %ecx	# save original EFLAGS
	xorl	$0x40000, %eax	# flip AC bit in EFLAGS
	pushl	%eax		# copy to EFLAGS
	popfl			# set EFLAGS
	pushfl			# get new EFLAGS
	popl	%eax		# put it in eax
	xorl	%ecx, %eax	# change in flags
	andl	$0x40000, %eax	# check if AC bit changed
	je	id_done

	movl	$4, X86(%esi)	# at least 486
	movl	%ecx, %eax
	xorl	$0x200000, %eax	# check ID flag
	pushl	%eax
	popfl			# if we are on a straight 486DX, SX, or
	pushfl			# 487SX we can't change it
	popl	%eax
	xorl	%ecx, %eax
	pushl	%ecx		# restore original EFLAGS
	popfl
	andl	$0x200000, %eax
	jne	have_cpuid

	/* Test for Cyrix CPU types */
	xorw	%ax, %ax	# clear ax
	sahf			# clear flags
	movw	$5, %ax
	movw	$2, %bx
	div	%bl		# do operation that does not change flags
	lahf			# get flags
	cmp	$2, %ah		# check for change in flags
	jne	id_done		# if not Cyrix
	movl	$2, X86(%esi)	# Use two to identify as Cyrix
	jmp	id_done

have_cpuid:
	/* get vendor info */
	xorl	%eax, %eax			# call CPUID with 0 -> return vendor ID
	cpuid
	movl	%eax, X86_CPUID(%esi)		# save CPUID level
	movl	%ebx, X86_VENDOR_ID(%esi)	# first 4 chars
	movl	%edx, X86_VENDOR_ID+4(%esi)	# next 4 chars
	movl	%ecx, X86_VENDOR_ID+8(%esi)	# last 4 chars

	orl	%eax, %eax			# do we have processor info as well?
	je	id_done

	movl	$1, %eax		# Use the CPUID instruction to get CPU type
	cpuid


	#
	# CDH start
	# Check FPU, initialize if present
	#
	testl $1, %edx # FPU available?
	jz no_fpu
	finit

	no_fpu:
	#
	# CDH end
	#

	movl  %eax, X86_EXT(%esi) # save complete extended CPUID to X86_EXT
	movl  %ecx, X86_FFL(%esi) # save ECX Feature Flags to X86_FFL
	movb	%al, %cl		# save reg for future use
	andb	$0x0f, %ah		# mask processor family
	movb	%ah, X86(%esi)
	andb	$0xf0, %al		# mask model
	shrb	$4, %al
	movb	%al, X86_MODEL(%esi)
	andb	$0x0f, %cl		# mask mask revision
	movb	%cl, X86_MASK(%esi)
	movl	%edx, X86_CAPABILITY(%esi)

	movl	$0, X86_CACHE(%esi)
	movl	$0, X86_CACHE+4(%esi)
	movl	$0, X86_CACHE+8(%esi)
	movl	$0, X86_CACHE+12(%esi)

	movl	X86_VENDOR_ID+8(%esi), %eax
	cmpl	$0x6c65746e,%eax	# Is this an Intel CPU? "GenuineIntel"
	jne	not_intel
	movb	%bl, X86_PWRCAP(%esi) 	# Store BrandID in AMD PWRCAP if the CPU is from Intel
	movl	$2, %eax		# Use the CPUID instruction to get cache info
	cpuid
	movl	%eax, X86_CACHE(%esi)
	movl	%ebx, X86_CACHE+4(%esi)
	movl	%ecx, X86_CACHE+8(%esi)
	movl	%edx, X86_CACHE+12(%esi)
# Grab deterministic cache information (for 32nm Intel CPU)
	cmpw $0x0000,%dx
	jne id_done
	movl $4, %eax
	movl $0, %ecx
	cpuid
	movl	%eax, X86_DCACHE0_EAX(%esi)
	movl	%ebx, X86_DCACHE0_EBX(%esi)
	movl	%ecx, X86_DCACHE0_ECX(%esi)
	movl	%edx, X86_DCACHE0_EDX(%esi)	
	movl $4, %eax
	movl $1, %ecx
	cpuid
	movl	%eax, X86_DCACHE1_EAX(%esi)
	movl	%ebx, X86_DCACHE1_EBX(%esi)
	movl	%ecx, X86_DCACHE1_ECX(%esi)
	movl	%edx, X86_DCACHE1_EDX(%esi)	
	movl $4, %eax
	movl $2, %ecx
	cpuid
	movl	%eax, X86_DCACHE2_EAX(%esi)
	movl	%ebx, X86_DCACHE2_EBX(%esi)
	movl	%ecx, X86_DCACHE2_ECX(%esi)
	movl	%edx, X86_DCACHE2_EDX(%esi)	
	movl $4, %eax
	movl $3, %ecx
	cpuid
	movl	%eax, X86_DCACHE3_EAX(%esi)
	movl	%ebx, X86_DCACHE3_EBX(%esi)
	movl	%ecx, X86_DCACHE3_ECX(%esi)
	movl	%edx, X86_DCACHE3_EDX(%esi)	
	jmp	id_done

not_intel:
	movl	X86_VENDOR_ID+8(%esi),%eax
	cmpl	$0x444d4163, %eax	# Is this an AMD CPU? "AuthenticAMD"
	jne	not_amd

	movl	$0x80000005, %eax	# Use the CPUID instruction to get cache info
	cpuid
	movl	%ecx, X86_CACHE(%esi)
	movl	%edx, X86_CACHE+4(%esi)
	movl	$0x80000006,%eax	# Use the CPUID instruction to get cache info
	cpuid
	movl	%ecx,X86_CACHE+8(%esi)
	movl	%edx,X86_CACHE+12(%esi)
	movl	$0x80000007,%eax	# Use the CPUID instruction to get AMD Powercap
	cpuid
	movl	%edx,X86_PWRCAP(%esi)

not_amd:
	movl	X86_VENDOR_ID+8(%esi), %eax
	cmpl	$0x3638784D, %eax	# Is this a Transmeta CPU? "GenuineTMx86"
	jne	not_transmeta

	movl	$0x80000000, %eax	# Use the CPUID instruction to check for cache info
	cpuid
	cmp	$6, %al			# Is cache info available?
	jb	id_done

	movl	$0x80000005, %eax	# Use the CPUID instruction to get L1 cache info
	cpuid
	movl	%ecx, X86_CACHE(%esi)
	movl	%edx, X86_CACHE+4(%esi)
	movl	$0x80000006, %eax	# Use the CPUID instruction to get L2 cache info
	cpuid
	movl	%ecx, X86_CACHE+8(%esi)

not_transmeta:
	movl	X86_VENDOR_ID+8(%esi), %eax
	cmpl	$0x64616574, %eax	# Is this a Via/Cyrix CPU? "CyrixInstead"
	jne	not_cyrix

	movl	X86_CPUID(%esi), %eax	# get CPUID level
	cmpl	$2, %eax		# Is there cache information available ?
	jne	id_done

	movl	$2, %eax		# Use the CPUID instruction to get cache info
	cpuid
	movl	%edx, X86_CACHE(%esi)

not_cyrix:
	movl	X86_VENDOR_ID+8(%esi), %eax
	cmpl	$0x736C7561, %eax	# Is this a Via/Centaur CPU "CentaurHauls"
	jne	not_centaur

	movl	$0x80000000, %eax	# Use the CPUID instruction to check for cache info
	cpuid
	cmp	$6, %al			# Is cache info available?
	jb	id_done

	movl	$0x80000005, %eax	# Use the CPUID instruction to get L1 cache info
	cpuid
	movl	%ecx, X86_CACHE(%esi)
	movl	%edx, X86_CACHE+4(%esi)
	movl	$0x80000006, %eax	# Use the CPUID instruction to get L2 cache info
	cpuid
	movl	%ecx, X86_CACHE+8(%esi)


not_centaur:
id_done:

	call	do_main
	/* In case we return simulate an exception */
	pushfl
	pushl	%cs
	call	0f
0:	pushl	$0 /* error code */
	pushl	$257 /* vector */
	jmp	int_hand

vec0:
	pushl	$0 /* error code */
	pushl	$0 /* vector */
	jmp int_hand
vec1:
	pushl	$0 /* error code */
	pushl	$1 /* vector */
	jmp int_hand

vec2:
	pushl	$0 /* error code */
	pushl	$2 /* vector */
	jmp int_hand

vec3:
	pushl	$0 /* error code */
	pushl	$3 /* vector */
	jmp	int_hand

vec4:
	pushl	$0 /* error code */
	pushl	$4 /* vector */
	jmp	int_hand

vec5:
	pushl	$0 /* error code */
	pushl	$5 /* vector */
	jmp	int_hand

vec6:
	pushl	$0 /* error code */
	pushl	$6 /* vector */
	jmp	int_hand

vec7:
	pushl	$0 /* error code */
	pushl	$7 /* vector */
	jmp	int_hand

vec8:
	/* error code */
	pushl	$8 /* vector */
	jmp	int_hand

vec9:
	pushl	$0 /* error code */
	pushl	$9 /* vector */
	jmp int_hand

vec10:
	/* error code */
	pushl	$10 /* vector */
	jmp	int_hand

vec11:
	/* error code */
	pushl	$11 /* vector */
	jmp	int_hand

vec12:
	/* error code */
	pushl	$12 /* vector */
	jmp	int_hand

vec13:
	/* error code */
	pushl	$13 /* vector */
	jmp	int_hand

vec14:
	/* error code */
	pushl	$14 /* vector */
	jmp	int_hand

vec15:
	pushl	$0 /* error code */
	pushl	$15 /* vector */
	jmp	int_hand

vec16:
	pushl	$0 /* error code */
	pushl	$16 /* vector */
	jmp	int_hand

vec17:
	/* error code */
	pushl	$17 /* vector */
	jmp	int_hand

vec18:
	pushl	$0 /* error code */
	pushl	$18 /* vector */
	jmp	int_hand

vec19:
	pushl	$0 /* error code */
	pushl	$19 /* vector */
	jmp	int_hand

int_hand:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

	/* original stack pointer */
	leal	20(%esp), %eax
	pushl	%eax

	pushl	%esp /* pointer to structure on the stack */
	call	inter
	addl	$8, %esp

	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret

/*
 * The interrupt descriptor table has room for 32 idt's
 */
.align 4
.word 0
idt_descr:
	.word 20*8-1	       # idt contains 32 entries
	.long 0

idt:
	.fill 20,8,0	       # idt is uninitialized

gdt_descr:
	.word 6*8 - 1
	.long 0


.align 4
.globl gdt, gdt_end, real_code_begin, real_code_end, query_pcbios
gdt:
	.quad 0x0000000000000000	/* NULL descriptor */
	.quad 0x0000000000000000	/* not used */
	.quad 0x00cf9a000000ffff	/* 0x10 main 4gb code at 0x000000 */
	.quad 0x00cf92000000ffff	/* 0x18 main 4gb data at 0x000000 */

	.word	0xFFFF				# 16bit 64KB - (0x10000*1 = 64KB)
	.word	0				# base address = SETUPSEG
	.byte	0x00, 0x9b			# code read/exec/accessed
	.byte	0x00, 0x00			# granularity = bytes


	.word	0xFFFF				# 16bit 64KB - (0x10000*1 = 64KB)
	.word	0				# base address = SETUPSEG
	.byte	0x00, 0x93			# data read/write/accessed
	.byte	0x00, 0x00			# granularity = bytes

gdt_end:
	.word	0
query_pcbios:
	/* Save the caller save registers */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp

	lidt	idt_real

	/* Don't disable the a20 line */

	/* Load 16bit data segments, to ensure the segment limits are set */
	movl	$REAL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %fs
	movl	%eax, %gs

	/* Save %esp */
	movl	%esp, %ebx

	/* switch to 16bit mode */
	ljmp	$REAL_CS, $1f - startup_32
1:
	.code16
	/* Disable Paging and protected mode */
	/* clear the PG & PE bits of CR0 */
	movl	%cr0,%eax
	andl	$~((1 << 31)|(1<<0)),%eax
	movl	%eax,%cr0

	/* make intersegment jmp to flush the processor pipeline
	 * and reload %cs:%eip (to clear upper 16 bits of %eip).
	 */
	ljmp	$REAL_CODE_SEG, $REAL_CODE_OFFSET

/* will be relocate to [1000]:[0000] */
real_code_begin:
	/* we are in real mode now
	 * set up the real mode segment registers : %ds, %ss, %es, %gs, %fs
	 */
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Adjust the stack pointer */
	xorl	%esp,%esp

	/* Make edi point to real_reg */
	movl	$REAL_CODE_SEG, %eax
	shll	$4, %eax
	xor	%eax, %edi

	/* Save my old stack pointer on real mode stack */
	pushl	%ebx

	/* Enable interrupts or BIOS's go crazy */
	sti
###################real code##################
# ds:di point to real_reg
	# load ax, bx, cx, dx
	movw %ds:0(%di), %ax
	movw %ds:2(%di), %bx
	movw %ds:4(%di), %cx
	movw %ds:6(%di), %dx
	movw %ds:8(%di), %es
	movw %ds:10(%di), %si
	pushw %di
	movw %ds:12(%di), %di

	int $0x13

	# save ax, bx, cx, dx, es, si, di is drop!
	# CF->ret
	jc 1f

	popw %di
	movl $0,%ds:14(%di)
	jmp 2f
1:
	popw %di
	movl $1,%ds:14(%di)
2:
	movw %ax, %ds:0(%di)
	movw %bx, %ds:2(%di)
	movw %cx, %ds:4(%di)
	movw %dx, %ds:6(%di) 
	movw %es, %ds:8(%di)
	movw %si, %ds:10(%di)
##############################################

	/* O.k. the BIOS query is done switch back to protected mode */
	cli

	/* Restore my old stack pointer */
	popl	%ebx
	movl	%ebx, %esp

	/* Get an convinient %ds */
	movw	%cs, %ax
	movw	%ax, %ds

	/* Load the global descriptor table */
	addr32 lgdt	gdt_real_descr - real_code_begin

	/* Turn on protected mode */
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

	/* flush the prefetch queue, and relaod %cs:%eip */
	data32 ljmp $KERNEL_CS, $prot

.align 4
.globl gdt_real, gdt_real_end, idt_real, gdt_real_descr
gdt_real_descr:
	.word	4*8 - 1
	.long	gdt_real - real_code_begin + REAL_CODE_ADDR

gdt_real:
	.quad	0x0000000000000000	/* NULL descriptor */
	.quad	0x0000000000000000	/* not used */
	.quad	0x00cf9a000000ffff	/* 0x10 main 4gb code at 0x000000 */
	.quad	0x00cf92000000ffff	/* 0x18 main 4gb data at 0x000000 */
gdt_real_end:

real_data:
	.globl real_reg, real_dap, real_disk_param_ext,real_buffer,real_param_ext
.balign 16
real_reg:
	.word 0  # ax
	.word 0  # bx
	.word 0  # cx
	.word 0  # dx
	.word 0  # es
	.word 0  # si
	.word 0  # di
	.long 0  # res

.balign 16
real_dap:
	.byte 0		# size of DAP = 16 = 10h
	.byte 0		# unused, should be zero
	.word 0		# number of sectors to be read
	.word 0		# offset of buffer
	.word 0         # base of buffer
	.long 0		# absolute number of the start of the sectors to be read (1st sector of drive has number 0)
	.long 0

.balign 16
real_disk_param_ext:
	.word 0			# size of Result Buffer = 30 = 1Eh
	.word 0			# information flags
	.long 0			# physical number of cylinders = last index + 1 (because index starts with 0)
	.long 0			# physical number of heads = last index + 1 (because index starts with 0)
	.long 0			# physical number of sectors per track = last index (because index starts with 1)
	.long 0			# absolute number of sectors = last index + 1 (because index starts with 0)
	.long 0
	.word 0			# bytes per sector
	.long 0			# optional pointer to Enhanced Disk Drive (EDD) configuration parameters
	                        # which may be used for subsequent interrupt 13h Extension calls (if supported)
.balign 16
real_buffer:
	.space	MAX_SECTOR_BUFFER_SIZE , 0
real_code_end:
	.word	0

idt_real:
	.word	0x400 - 1			# idt limit ( 256 entries)
	.word	0, 0				# idt base = 0L

prot:
	.code32
	/* Reload gdt, then reload other segment registers */
	lgdt	gdt_descr
	leal	1f, %eax
	pushl	$KERNEL_CS
	pushl	%eax
	lret
1:	movl	$KERNEL_DS, %eax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Restore the caller saved registers */
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	movl	$1, %eax
	ret


.data 
.globl cmd_line_ptr
zerobss:	.long	1
clear_display:	.long	1
cmd_line_ptr:	.long	1
.bss
.balign 16
stack:
	. = . + PROT_STACK_SIZE
stack_top:
