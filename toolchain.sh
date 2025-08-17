#!/bin/bash
set -e

# Script based on:
# https://wiki.osdev.org/GCC_Cross-Compiler
# https://wiki.osdev.org/Building_libgcc_for_mcmodel=kernel

BUILD_BINUTILS=1
BUILD_GCC=1
BUILD_GRUB=1
BUILD_MISSING=1

URL_BINUTILS="https://ftp.gnu.org/gnu/binutils/binutils-2.43.tar.xz"
URL_GCC="https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz"
URL_GRUB="https://ftp.gnu.org/gnu/grub/grub-2.02.tar.xz"

export PREFIX="$HOME/osdev/x86_64"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

if [ "$1" == "--all" ]; then
    BUILD_MISSING=0
fi

if [[ $BUILD_MISSING -ne 0 ]]; then
    if [ -f "$PREFIX/bin/x86_64-elf-ld" ]; then
        BUILD_BINUTILS=0
    fi
    if [ -f "$PREFIX/bin/x86_64-elf-gcc" ]; then
        BUILD_GCC=0
    fi
    if [ -f "$PREFIX/bin/grub-mkrescue" ]; then
        BUILD_GRUB=0
    fi
fi

if [[ $BUILD_BINUTILS -ne 0 ]]; then
    mkdir -p src
    cd src

    echo -e "\e[32mBuilding package $(basename $URL_BINUTILS)\e[0m"
    wget -nc $URL_BINUTILS
    tar xf binutils-*.tar.xz

    ./binutils-*/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
    make -j6 MAKEINFO=true
    make install MAKEINFO=true

    cd ..
    rm -rf src
fi

# Requires: libmpc-dev libgmp-dev libmpfr-dev
if [[ $BUILD_GCC -ne 0 ]]; then
    mkdir -p src
    cd src

    echo -e "\e[32mBuilding package $(basename $URL_GCC)\e[0m"
    wget -nc $URL_GCC
    tar xf gcc-*.tar.xz

    ./gcc-*/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers
    make -j6 all-gcc
    make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone' || true
    sed -i 's/PICFLAG/DISABLED_PICFLAG/g' $TARGET/libgcc/Makefile
    make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone'
    make install-gcc
    make install-target-libgcc

    cd ..
    rm -rf src
fi

# Requires: bison flex
if [[ $BUILD_GRUB -ne 0 ]]; then
    mkdir -p src
    cd src

    echo -e "\e[32mBuilding package $(basename $URL_GRUB)\e[0m"
    wget -nc $URL_GRUB
    tar xf grub-*.tar.xz

    ./grub-*/configure --disable-efiemu --disable-werror TARGET_CC=$TARGET-gcc TARGET_OBJCOPY=$TARGET-objcopy TARGET_STRIP=$TARGET-strip TARGET_NM=$TARGET-nm TARGET_RANLIB=$TARGET-ranlib --target=$TARGET --prefix=$PREFIX
    make -j6
    make install

    cd ..
    rm -rf src
fi
