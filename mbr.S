#define __ASSEMBLY__
#include "defs.h"

.code16
.section ".mbr", "ax", @progbits
.globl _boot
_boot:
	jmp start
message:
	.asciz "there", "are", "two", "tigers.\r\n"
	.asciz "there", "are", "two", "tigers.\r\n"
	.asciz "run", "run", "run.\r\n"
	.asciz "run", "run", "run.\r\n"
	.asciz "one", "of", "them", "has", "no", "eyes.\r\n"
	.asciz "one", "of", "them", "has", "no", "ears.\r\n"
	.asciz "how", "fucking", "weird!\r\n"
	.asciz "how", "fucking", "weird!\r\n"
	.asciz "+"
freq: 
	.word 262,294,330,262
	.word 262,294,330,262
	.word 330,349,392
	.word 330,349,392
	.word 392,440,392,349,330,262
	.word 392,440,392,349,330,262
	.word 294,196,262
	.word 294,196,262
	.word 0

delay: 
	.byte 8,8,8,8,8,8,8,8,8,8
	.byte 16,8,8,16
	.byte 4,4,4,4,8,8
	.byte 4,4,4,4,8,8
	.byte 8,8,16
	.byte 8,8,16
start:
	cli
	movw	$BOOTSEG, %ax
	movw	%ax, %ds
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$256, %cx
	subw	%si, %si
	subw	%di, %di
	cld
	rep
	movsw
	ljmp	$INITSEG, $go - _boot

go:
	movw	%cs, %ax
	movw	$(0x4000-12), %dx

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss		# put stack at INITSEG:0x4000-12.
	movw	%dx, %sp

	mov $' ', %al			# display one space
	mov $0x1, %bx
	mov $0xe, %ah 
	int $0x10
main:
	call load_string_table
	call play_music
	jmp  main

play_music:
	pushw %di			# save context
	pushw %si
	pushw %bp
	pushw %bx


	movw $freq - _boot, %si
	movw $delay - _boot, %bp
	movw $msg_table - _boot, %di


	movw $0x61, %dx			# enable speaker
	inb %dx, %al
	orb $0x03, %al
	outb %al, %dx

	movw $0x43, %dx			# program pit
	movb $0xb6, %al			# channel 2(speaker), lobyte/hibyte, square wave, 16bits
	outb %al, %dx
pp:

	movw (%si),%bx			# get one freq to di
	cmp $0, %bx			# is end ?
	je end_play			# if so, end play

	pushw %si
	movw (%di), %si
	call print_str			# print word
	popw %si

	movw (%si),%bx
	movw $0x12, %dx			# freq to counter of 8254
	movw $0x34dc, %ax		# 1.19318MHz
	divw %bx
	movw $0x42, %dx			# load counter to 8254
	outb %al, %dx			# low bytes first
	movb %ah, %al
	outb %al, %dx			# hi bytes

	movb %ds:(%bp), %bl             # get one delay value into bx
	xorb %bh, %bh
	call time_delay			# call delay sub proc
	add $2, %si			# get next freq
	add $1, %bp			# get next delay time
	add $2, %di			# get next message
	jmp pp				# continue
 
end_play:
	movw $0x61, %dx			# shut down speaker
	inb %dx, %al
	andb $0xfc, %al
	outb %al, %dx

	popw %bx			# restore context
	popw %bp
	popw %si
	popw %di

	ret

time_delay:
	movw %bx, %cx
	movw $0x43, %dx			# program pit
	movb $0x30, %al			# channel 0(irq0), lobyte/hibyte, hardware re-triggerable one-shot, 16bits
	outb %al, %dx
2:
	movw $0xffff, %ax
	movw $0x40, %dx			# load counter to 8254
	outb %al, %dx			# low bytes first
	movb %ah, %al
	outb %al, %dx			# hi bytes
1:
	# read back status
	# 11100010
	movb $0xe2, %al
	movw $0x43, %dx	
	outb %al, %dx
	
	movw $0x40, %dx
	inb %dx, %al			# read status, until bits 7 set
	movb $0, %ah

	andb $0x80, %al
	jz 1b

	decw %cx
	jnz 2b
	ret

load_string_table:
	movw $msg_table - _boot, %di
	movw $message - _boot, %si
2:
	movw %si, (%di)
1:
	lodsb
	cmp $'+', %al
	jz 3f
	cmp $0x0, %al
	jnz 1b
	addw $2, %di
	jmp 2b
3:
	ret;

#Displays a character
#int 0x10 ah=e
#al = character, bh = page number
char:
	mov $0x1, %bx
	mov $0xe, %ah 
	int $0x10
print_str:
	lodsb				# lodsb loads ds:esi into al
	cmp $0x0, %al 
	jnz char			# display character if not null
	mov $' ', %al			# display space
	mov $0x1, %bx
	mov $0xe, %ah 
	int $0x10
	ret

.org 510
.word 0xAA55
msg_table: