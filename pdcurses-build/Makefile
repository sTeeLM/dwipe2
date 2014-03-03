# GNU MAKE (3.79.1) Makefile for PDCurses library - DOS DJGPP V2.0+
#
# Usage: make -f [path\]gccdos.mak [DEBUG=Y] [target]
#
# where target can be any of:
# [all|libs|demos|dist|pdcurses.a|testcurs.exe...]

O = o

ifndef PDCURSES_SRCDIR
	PDCURSES_SRCDIR = ../pdcurses
endif

include $(PDCURSES_SRCDIR)/version.mif
include $(PDCURSES_SRCDIR)/libobjs.mif

osdir		= $(PDCURSES_SRCDIR)/minios

PDCURSES_DOS_H	= $(osdir)/pdcminios.h

CC		= gcc

ifeq ($(DEBUG),Y)
	CFLAGS  = -g -Wall -DPDCDEBUG
	LDFLAGS = -g
else
	CFLAGS  = -g -O2 -Wall -march=i486 -m32 -O2 -fomit-frame-pointer -fno-builtin -ffreestanding -fno-stack-protector -I ../newlib-output/i386-elf/include/
	LDFLAGS = -g -L ../newlib-output/i386-elf/lib/ -lc -lm -lminios
endif

CFLAGS += -I$(PDCURSES_SRCDIR)

INSTALL_DIR = ../pdcurses-output/

LINK		= gcc

LIBEXE		= ar
LIBFLAGS	= rcv

LIBCURSES	= pdcurses.a

.PHONY: all libs clean demos dist

all:	libs

libs:	$(LIBCURSES)

clean:
	rm -rf *.o
	rm -rf *.a

demos:	$(DEMOS)
	strip *.exe

$(LIBCURSES) : $(LIBOBJS) $(PDCOBJS)
	$(LIBEXE) $(LIBFLAGS) $@ $?

$(LIBOBJS) $(PDCOBJS) : $(PDCURSES_HEADERS)
$(PDCOBJS) : $(PDCURSES_DOS_H)
$(DEMOS) : $(PDCURSES_CURSES_H) $(LIBCURSES)
panel.o : $(PANEL_HEADER)
terminfo.o: $(TERM_HEADER)

$(LIBOBJS) : %.o: $(srcdir)/%.c
	$(CC) -c $(CFLAGS) $<

$(PDCOBJS) : %.o: $(osdir)/%.c
	$(CC) -c $(CFLAGS) $<

firework.exe newdemo.exe rain.exe testcurs.exe worm.exe xmas.exe \
ptest.exe: %.exe: $(demodir)/%.c
	$(CC) $(CFLAGS) -o$@ $< $(LIBCURSES)

tuidemo.exe: tuidemo.o tui.o
	$(LINK) $(LDFLAGS) -o$@ tuidemo.o tui.o $(LIBCURSES)

tui.o: $(demodir)/tui.c $(demodir)/tui.h $(PDCURSES_CURSES_H)
	$(CC) -c $(CFLAGS) -I$(demodir) -o$@ $<

tuidemo.o: $(demodir)/tuidemo.c $(PDCURSES_CURSES_H)
	$(CC) -c $(CFLAGS) -I$(demodir) -o$@ $<

PLATFORM1 = DJGPP 2.03
PLATFORM2 = DJGPP 2.03 for DOS
ARCNAME = pdc$(VER)djg

install:
	mkdir -p $(INSTALL_DIR)/include
	mkdir -p $(INSTALL_DIR)/lib
	cp $(LIBCURSES) $(INSTALL_DIR)/lib
	cp $(PDCURSES_SRCDIR)/minios/pdcminios.h $(INSTALL_DIR)/include
	cp $(PDCURSES_SRCDIR)/*.h $(INSTALL_DIR)/include
