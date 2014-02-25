#!/bin/sh
losetup /dev/loop0 /win/boot/boot.img -o$((512*17))
mount /dev/loop0 /mnt
/bin/cp memtest.bin /mnt/isolinux/
umount /mnt
losetup -d /dev/loop0
