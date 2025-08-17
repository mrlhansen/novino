#!/bin/bash

export SABI=${PWD}/sabi
mkdir -p ${SABI}/include

mkdir tmp
cd tmp
git clone https://github.com/mrlhansen/sabi.git

cd sabi
rm -f source/host.c
cp -f source/*.c ${SABI}
cp -f include/sabi/*.h ${SABI}/include

cd ../..
rm -rf tmp
