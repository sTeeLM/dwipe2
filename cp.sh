#!/bin/sh
losetup /dev/loop0 /win/boot/boot.img -o$((512*17))
mount /dev/loop0 /mnt
/bin/cp minios.bin /mnt/isolinux/
/bin/cp mbr.bin /mnt/isolinux/mbr.bs
umount /mnt
losetup -d /dev/loop0
