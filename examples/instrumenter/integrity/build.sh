#!/bin/sh

../../../build_passes.sh

export LLVM_COMPILER=clang
export CC=clang
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so   --verbose"
make
