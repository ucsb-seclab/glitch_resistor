#!/bin/sh
  
export LLVM_COMPILER=clang
export CLANG_FLAGS="-Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
make
