# Makefile for MemTest86+
#
# Author:		Chris Brady
# Created:		January 1, 1996

#
# Path for the floppy disk device
#
FDISK=/dev/fd0

AS=as -32
CC=gcc

CFLAGS= -Wall -march=i486 -m32 -O2 -fomit-frame-pointer -fno-builtin -ffreestanding -fno-stack-protector \
        -I newlib-output/i386-elf/include/
LDFLAGS= -L newlib-output/i386-elf/lib

LIBS= -lc -lm -lminios

OBJS= head.o int13.o main.o inter.o lib.o display.o disk.o test.o

all: memtest.bin memtest

# Link it statically once so I know I don't have undefined
# symbols and then link it dynamically so I have full
# relocation information
memtest_shared: $(OBJS) memtest_shared.lds Makefile
	$(LD) $(LDFLAGS) --warn-constructors --warn-common -static -T memtest_shared.lds \
	-o $@ $(OBJS) $(LIBS)

memtest_shared.bin: memtest_shared
	objcopy -O binary $< memtest_shared.bin

memtest: memtest_shared.bin memtest.lds
	$(LD) -s -T memtest.lds -b binary memtest_shared.bin -o $@

head.s: head.S defs.h
	$(CC) -E -traditional $< -o $@

bootsect.s: bootsect.S defs.h
	$(CC) -E -traditional $< -o $@

setup.s: setup.S defs.h
	$(CC) -E -traditional $< -o $@

int13.s: int13.S defs.h
	$(CC) -E -traditional $< -o $@

memtest.bin: memtest_shared.bin bootsect.o setup.o memtest.bin.lds
	$(LD) -T memtest.bin.lds bootsect.o setup.o -b binary \
	memtest_shared.bin -o memtest.bin

reloc.o: reloc.c
	$(CC) -c $(CFLAGS) -fno-strict-aliasing reloc.c

test.o: test.c
	$(CC) -c -Wall -march=i486 -m32 -Os -fomit-frame-pointer -fno-builtin -ffreestanding test.c

clean:
	rm -f *.o *.s *.iso memtest.bin memtest memtest_shared memtest_shared.bin

asm:
	@./makedos.sh

iso:
	make all
	./makeiso.sh
	rm -f *.o *.s memtest.bin memtest memtest_shared memtest_shared.bin

install: all
	dd <memtest.bin >$(FDISK) bs=8192

install-precomp:
	dd <precomp.bin >$(FDISK) bs=8192
	
dos: all
	cat mt86+_loader memtest.bin > memtest.exe

