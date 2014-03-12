#!/bin/sh

# check to see if the correct tools are installed
for X in wc mkisofs
do
	if [ "$(which $X)" = "" ]; then
		echo "makeiso.sh error: $X is not in your path." >&2
		exit 1
	elif [ ! -x $(which $X) ]; then
		echo "makeiso.sh error: $X is not executable." >&2
		exit 1
	fi 
done

#check to see if minios.bin is present
if [ ! -w minios.bin ]; then 
	echo "makeiso.sh error: cannot find minios.bin, did you compile it?" >&2 
	exit 1
fi


echo "Generating iso image ..."

mkdir "cd"
mkdir "cd/isolinux"
cp minios.bin cd/isolinux
cp isolinux.bin cd/isolinux
cp syslinux.cfg cd/isolinux

mkisofs -o mo420.iso \
                -b isolinux/isolinux.bin -c isolinux/boot.cat \
                -no-emul-boot -boot-load-size 4 -boot-info-table \
                cd

rm -rf cd

echo "Done! Minios 1.0.0 ISO is mo420.iso"
