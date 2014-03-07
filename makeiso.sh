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


# enlarge the size of minios.bin
SIZE=$(wc -c minios.bin | awk '{print $1}')
FILL=$((1474560 - $SIZE))
dd if=/dev/zero of=fill.tmp bs=$FILL count=1
cat minios.bin fill.tmp > minios.img
rm -f fill.tmp

echo "Generating iso image ..."

mkdir "cd"
mkdir "cd/boot"
mv minios.img cd/boot
cd cd

# Create the cd.README
echo -e "There is nothing to do here\r\r\nMinios is located on the bootsector of this CD\r\r\n" > README.TXT
echo -e "Just boot from this CD and Minios will launch" >> README.TXT

mkisofs -A "MKISOFS 1.1.2" -p "Minios 1.0.0" -publisher "sTeel<steel.mental@gmail.com>" -b boot/minios.img -c boot/boot.catalog -V "MT410" -o minios.iso .
mv minios.iso ../mt420.iso
cd ..
rm -rf cd

echo "Done! Minios 1.0.0 ISO is mo420.iso"
