#!/bin/bash

../../../build_passes.sh

 if [ $# -eq 2 ]; then
	echo "Selecting function $2"
	function=$2
else
	function=None
fi

export LLVM_CC=clang
export CPATH=../../../runtime/gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include/  
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so"
if [ "$1" != "NODELAY" ]; then
	export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
else
	echo "Compiling without the Delay module"
fi
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/RecheckLoopProtector/libRecheckLoopProtector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so"
export GR_ENUM_FLAGS="-extra-arg-before=-w --allinit -output-postfix bak"
if [ "$1" == "NODEFENSE" ]; then
	echo "Disabling all defenses..."
	export CLANG_FLAGS=""
	export GR_ENUM_FLAGS=""
fi
make clean
make FUNC_SEL=$function
