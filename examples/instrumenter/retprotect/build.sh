#!/bin/sh

export CC=clang
export CFLAGS="-Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so --verbose"
make
