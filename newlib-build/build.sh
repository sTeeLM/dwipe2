#!/bin/sh
workdir=`pwd`
$workdir/../newlib/configure --target=i386-elf --enable-languages=c --prefix=$workdir/../newlib-output
