#!/bin/sh

../../../build_passes.sh || exit -1

export LLVM_COMPILER=clang
export CC=clang
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/LoopProtector/libLoopProtector.so"
make clean
make
