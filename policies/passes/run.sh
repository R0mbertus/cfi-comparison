#!/bin/bash

mkdir -p build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make
