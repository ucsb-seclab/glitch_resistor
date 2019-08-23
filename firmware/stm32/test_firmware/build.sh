#!/bin/sh

export CPATH=../../../runtime//gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include/  
#export LLVM_CC=clang
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/LoopProtector/libLoopProtector.so"
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so"
make
