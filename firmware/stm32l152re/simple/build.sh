#!/bin/sh

../../../build_passes.sh
  
export LLVM_COMPILER=clang
export CC=wllvm
export CLANG_FLAGS+="-Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so "
export CLANG_FLAGS+="-Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so "
CC=clang make
