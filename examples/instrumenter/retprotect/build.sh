#!/bin/sh

export LLVM_COMPILER=clang
export CC=wllvm
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so --verbose"
make
