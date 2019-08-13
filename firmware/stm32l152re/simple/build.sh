#!/bin/sh
  
export LLVM_COMPILER=clang
export CC=wllvm
export CLANG_FLAGS="-Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so --verbose"
CC=clang make
