#!/bin/sh

cd `dirname $0`
cd instrumenter/

mkdir build
cd build
cmake ..
make
