# Makefile for MINIOS
#


AS=as -32
CC=gcc

CFLAGS= -Wall -march=i486 -m32 -O2 -fomit-frame-pointer -fno-builtin -ffreestanding -fno-stack-protector \
        -I newlib-output/i386-elf/include/
LDFLAGS= -L newlib-output/i386-elf/lib

LIBS= -lc -lm -lminios

OBJS= head.o int13.o dwipe.o inter.o lib.o display.o disk.o serial.o cmdline.o debug.o timer.o option.o fat.o

all: minios.bin minios

# Link it statically once so I know I don't have undefined
# symbols and then link it dynamically so I have full
# relocation information
minios_shared: $(OBJS) minios_shared.lds Makefile
	$(LD) $(LDFLAGS) --warn-constructors --warn-common -static -T minios_shared.lds \
	-o $@ $(OBJS) $(LIBS)

minios_shared.bin: minios_shared
	objcopy -O binary $< minios_shared.bin

minios: minios_shared.bin minios.lds
	$(LD) -s -T minios.lds -b binary minios_shared.bin -o $@

head.s: head.S defs.h
	$(CC) -E -traditional $< -o $@

bootsect.s: bootsect.S defs.h
	$(CC) -E -traditional $< -o $@

setup.s: setup.S defs.h
	$(CC) -E -traditional $< -o $@

int13.s: int13.S defs.h
	$(CC) -E -traditional $< -o $@

minios.bin: minios_shared.bin bootsect.o setup.o minios.bin.lds
	$(LD) -T minios.bin.lds bootsect.o setup.o -b binary \
	minios_shared.bin -o minios.bin

mbr.s: mbr.S defs.h
	$(CC) -E -traditional $< -o $@

mbr.bin: mbr.o
	$(LD) -T mbr.lds mbr.o -o $@

dwipe.o: mbr.inc

mbr.inc: mbr.bin
	xxd -i mbr.bin > mbr.inc

clean:
	rm -f *.o *.s *.iso *.inc mbr.bin minios.bin minios minios_shared minios_shared.bin

iso:
	make all
	./makeiso.sh
	rm -f *.o *.s *.inc minios.bin minios minios_shared minios_shared.bin
