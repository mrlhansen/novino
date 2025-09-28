#!/bin/bash

# exit on error
set -e

# import variables
source env.sh

# build libc for kernel space
export CCFLAGS="${KS_CCFLAGS} -D__KERNEL__"
make clean -C libc
make -C libc
mv ${SYSROOT}${LIBDIR}/libc.a ${SYSROOT}${LIBDIR}/libk.a

# build libc for user space
export CCFLAGS=${US_CCFLAGS}
make clean -C libc
make -C libc
