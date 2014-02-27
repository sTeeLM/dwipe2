/*
 *  int13.S contains the 32-bit int 13 call code.
 *
 */
#define __ASSEMBLY__
#include "defs.h"

.text
.code32
.globl int13_1, int13_2, int13_3, int13_4

/*
caller fill ax, bx, cx, dx of real_reg, zero es,si,di of real_reg, ret is in real_ret
2.3 INT 13h AH=00h: Reset Disk Drive
2.4 INT 13h AH=01h: Get Status of Last Drive Operation
2.10 INT 13h AH=07h: Format Drive Starting at Track
2.11 INT 13h AH=08h: Read Drive Parameters
2.12 INT 13h AH=09h: Init Drive Pair Characteristics
2.13 INT 13h AH=0Ah: Read Long Sectors From Drive
2.14 INT 13h AH=41h: Check Extensions Present
*/

int13_1:
	pushl	%edi
	pushl	%ebx
	subl	$0x8, %esp
	movl	p_real_reg, %edi
	movw	$0, 8(%edi)   # zero es
	movw	$0, 10(%edi)  # zero si
	movw	$0, 12(%edi)  # zero di
	call	query_pcbios
	movl	p_real_ret, %edi
	movl	(%edi), %eax
	addl	$0x8, %esp
	pop	%ebx
	pop	%edi
	ret
/*
 caller fill ax, bx, cx, dx of real_reg, we set es:bx->real_buffer, zero di, ret is in real_ret
 2.5 INT 13h AH=02h: Read Sectors From Drive 
	ES:BX	Buffer Address Pointer
 2.6 INT 13h AH=03h: Write Sectors To Drive
	ES:BX	Buffer Address Pointer
 2.7 INT 13h AH=04h: Verify Sectors From Drive
	ES:BX	Buffer Address Pointer
 2.8 INT 13h AH=05h: Format Track
	ES:BX	Buffer Address Pointer$
*/
int13_2:
	pushl	%esi
	pushl	%edi	
	pushl	%ebx
	subl	$0x8, %esp
	movl	p_real_reg, %edi
	movl	p_real_buffer, %esi
	movl	$REAL_CODE_SEG, %eax
	shll	$4, %eax
	xorl	%eax, %esi
	movw	$REAL_CODE_SEG, 8(%edi)   # set es as base
	movw	$0, 10(%edi)   # zero si
	movw	$0, 12(%edi)  # zero di
	movw	%si, 2(%edi)   # set bx as offset
	call	query_pcbios
	movl	p_real_ret, %edi
	movl	(%edi), %eax
	addl	$0x8, %esp
	popl	%ebx
	popl	%edi
	popl	%esi
	ret
/*
 caller fill ax, bx, cx, dx, we set ds:si->real_dap, zero di, buffer of real_dap->real_buffer, ret is in real_ret
 2.15 INT 13h AH=42h: Extended Read Sectors From Drive
	DS:SI	segment:offset pointer to the DAP
 2.16 INT 13h AH=43h: Extended Write Sectors to Drive
	DS:SI	segment:offset pointer to the DAP
*/
int13_3:
#jmp	int13_3
	pushl	%esi
	pushl	%edi
	pushl	%ecx
	pushl	%ebx
	subl	$0x8, %esp
	movl	p_real_reg, %edi
	movl	p_real_buffer, %esi
	movl	p_real_dap, %ecx
	movl	%esi, %eax
	andl	$0x0000ffff, %eax
	andl	$0x000f0000, %esi
	shrl	$4, %esi
	movb    $0x10, (%ecx)  # size = 0x10
	movb    $0, 1(%ecx)    # unused = 0
	movw	%ax, 4(%ecx)   # set offset in dap
	movw	%si, 6(%ecx)   # set base in dap
	movl	%ecx, %eax
	andl	$0x0000ffff, %eax
	movw	%ax, 10(%edi)   # set si
	movw	$0, 12(%edi)    # zero di
	call	query_pcbios
	movl	p_real_ret, %edi
	movl	(%edi), %eax
	addl	$0x8, %esp
	popl	%ebx
	popl	%ecx
	popl	%edi
	popl	%esi
	ret

/*
caller fill ax, bx, cx, dx, we set ds:si->real_disk_param_ext, zero di, ret is in real_ret
2.17 INT 13h AH=48h: Extended Read Drive Parameters
	DS:SI	segment:offset pointer to Result Buffer, see below
*/
int13_4:
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	subl	$0x8, %esp
	movl	p_real_reg, %edi
	movl	p_real_disk_param_ext, %esi
	movw	$0x1e, 0(%esi)  # size = x1e
	movl	%esi, %eax
	andl	$0x0000ffff, %eax
	andl	$0x000f0000, %esi
	shrl	$16, %esi
	movw	%ax, 10(%edi)   # set si
	movw	$0, 12(%edi)    # zero di
	call	query_pcbios
	movl	p_real_ret, %edi
	movl	(%edi), %eax
	addl	$0x8, %esp
	popl	%ebx
	popl	%edi
	popl	%esi
	ret
