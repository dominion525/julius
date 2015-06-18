#!/bin/sh
#
# Build all binaries under directory "build-bin".
#
# This should be invoked at "..", top of the source archive.
#
# argument: any configure options except "--enable-setup=...",
# "--enable-julian" is allowed.
# 
VERSION=3.5.3

######################################################################

mkdir build-bin
dir=`pwd`

# make julius and other tools with default setting
./configure $* --bindir=${dir}/build-bin
make
make install.bin

# make julius/julian with another setting
rm ${dir}/build-bin/julius
cd julius
make install.bin INSTALLTARGET=julius-${VERSION}

# julius-standard
make distclean
./configure $* --bindir=${dir}/build-bin --enable-setup=standard
make install.bin INSTALLTARGET=julius-${VERSION}-std

# julius-graphout
make distclean
./configure $* --bindir=${dir}/build-bin --enable-graphout
make install.bin INSTALLTARGET=julius-${VERSION}-graphout

# julius-standard-graphout
make distclean
./configure $* --bindir=${dir}/build-bin --enable-setup=standard --enable-graphout
make install.bin INSTALLTARGET=julius-${VERSION}-std-graphout

# julian
make distclean
./configure $* --bindir=${dir}/build-bin --enable-julian
make
make install.bin INSTALLTARGET=julian-${VERSION}

# julian-standard
make distclean
./configure $* --bindir=${dir}/build-bin --enable-julian --enable-setup=standard
make install.bin INSTALLTARGET=julian-${VERSION}-std

# julian-graphout
make distclean
./configure $* --bindir=${dir}/build-bin --enable-julian --enable-graphout
make install.bin INSTALLTARGET=julian-${VERSION}-graphout

# julian-standard-graphout
make distclean
./configure $* --bindir=${dir}/build-bin --enable-julian --enable-setup=standard --enable-graphout
make install.bin INSTALLTARGET=julian-${VERSION}-std-graphout

# finished
cd ..
make distclean
strip build-bin/*
ls build-bin
echo '###### FINISHED ######'
