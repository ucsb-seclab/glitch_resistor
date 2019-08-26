#!/bin/sh

../../../build_passes.sh

export LLVM_COMPILER=clang
export CC=wllvm
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so"
make clean
make

