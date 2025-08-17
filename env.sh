#!/bin/bash

# macOS
export PATH=${HOME}/cross/x86_64/bin:/opt/homebrew/bin:/opt/homebrew/opt/coreutils/libexec/gnubin:/opt/homebrew/opt/findutils/libexec/gnubin:${PATH}

# Linux
# export PATH=${HOME}/osdev/x86_64/bin:${PATH}

# Environment
export HOST=x86_64-elf
export LD=${HOST}-ld
export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc
export NASM=nasm

export PREFIX=/usr
export BOOTDIR=/boot
export LIBDIR=${PREFIX}/lib
export INCLUDEDIR=${PREFIX}/include

export SYSROOT="$(pwd)/sysroot"
export CC="${CC} --sysroot=${SYSROOT} -isystem=${INCLUDEDIR}"
export CCFLAGS="-Wall -O -ffreestanding -mno-red-zone -fno-omit-frame-pointer"
export LDFLAGS="-z max-page-size=0x1000 -nostdlib"

# Kernel space
export KS_CCFLAGS="${CCFLAGS} -mcmodel=kernel -mno-sse2"

# User space
export US_CCFLAGS="${CCFLAGS} -mcmodel=small"
