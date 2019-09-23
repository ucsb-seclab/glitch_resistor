#!/bin/sh

../../../build_passes.sh

export LLVM_COMPILER=clang
export CC=clang
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so --verbose -S -emit-llvm"
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so"

make
