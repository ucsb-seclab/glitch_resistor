#!/bin/bash

../../../build_passes.sh

export LLVM_CC=clang
export CPATH=../../../runtime//gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include/  
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so"
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/LoopProtector/libLoopProtector.so"
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so"
#export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so"
export GR_ENUM_FLAGS="-extra-arg-before=-w --allinit -output-postfix bak"
make
