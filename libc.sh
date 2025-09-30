#!/bin/bash

# exit on error
set -e

# import variables
source env.sh

# build libc for user space
export CCFLAGS=${US_CCFLAGS}
make clean -C libc
make -C libc
