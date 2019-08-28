#!/bin/sh

cd `dirname $0`

# Soure rewriter
cd llvm-8.0.0.obj
cmake --build . --target gp-source-rewriter
cd ..

# LLVM passes
cd instrumenter/
mkdir build
cd build
cmake ..
make
