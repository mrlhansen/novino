#!/bin/bash

# exit on error
set -e

# import variables
source env.sh
export CCFLAGS=${KS_CCFLAGS}

# make sysroot directories
mkdir -p ${SYSROOT}${LIBDIR}
mkdir -p ${SYSROOT}${BOOTDIR}
mkdir -p ${SYSROOT}${INCLUDEDIR}

# copy header files
for src in $(find kernel loader libc sabi -name '*.h'); do
dst=$(echo $src | sed -E 's:(libc|include|main)/::g')
install -D $src ${SYSROOT}${INCLUDEDIR}/$dst
done

# compile libc if needed
if [ -z "$1" ]; then
test -f libc/libc.a || ./libc.sh
fi

# clone sabi if needed
if [ -z "$1" ]; then
test -f sabi/parser.c || ./sabi.sh
fi

# compile sabi
make $1 -C sabi

# compile kernel
for path in $(find kernel -name Makefile -printf '%h\n' | sort -r); do
make $1 -C $path
done

# compile loader
make $1 -C loader/multiboot2
make $1 -C loader

# compile applications
export CCFLAGS=${US_CCFLAGS}
make $1 -C apps

# build initrd
tar -c --format=ustar -f ${SYSROOT}${BOOTDIR}/initrd.tar apps
