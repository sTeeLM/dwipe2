dwipe2
======

dwipe2 base on mini kernel（minios）

exsample syslinux.cfg:


default dwipe
implicit 1
prompt   1
timeout  1

label dwipe
   linux minios.bin
   append skip=MINIME debug=15 mode=ocd force_chs
# options:
# skip=xxx: skip FAT16/FAT32 label xxx, default MINIME
# debug=0~15: show debug message on com1, default 0
# test: if set, only read disk, no wiping
# check: if set, wipe with check, set on OCD mode
# force_chs: if set, wipe with C/H/S mode, only 2GB space wiped!
# mbr: if set, write a mbr play 'two tiger' song
# mode=[FAST|DOD|OCD]: wipe with zero one pass|wipe with 00/FF/00 3 pass|wipe with 00/FF/00/RAND 4 pass
