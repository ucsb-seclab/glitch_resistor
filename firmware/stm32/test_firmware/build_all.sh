#!/bin/sh

../../../build_passes.sh

export LLVM_CC=clang
export CPATH=../../../runtime//gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include/  

mkdir -p output

export CLANG_FLAGS=""
make clean
make > output/none 2>&1
cp build/test_firmware.elf output/none.elf

export GR_ENUM_FLAGS="-extra-arg-before=-w --allinit -output-postfix bak"
export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
make clean
make > output/delay 2>&1
cp build/test_firmware.elf output/delay.elf

export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so"
make clean
make > output/branch 2>&1
cp build/test_firmware.elf output/branch.elf

export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/LoopProtector/libLoopProtector.so"
make clean
make > output/loop 2>&1
cp build/test_firmware.elf output/loop.elf

export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so"
make clean
make > output/integrity 2>&1
cp build/test_firmware.elf output/integrity.elf

export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so"
make clean
make > output/ret 2>&1
cp build/test_firmware.elf output/ret.elf

export CLANG_FLAGS=""
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/DelayInjector/libDelayInjector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/BranchProtector/libBranchProtector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/LoopProtector/libLoopProtector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/IntegrityProtector/libIntegrityProtector.so"
export CLANG_FLAGS+=" -Xclang -load -Xclang ../../../instrumenter/build/RetProtector/libRetProtector.so"
export GR_ENUM_FLAGS="-extra-arg-before=-w --allinit -output-postfix bak"
make clean
make > output/all 2>&1
cp build/test_firmware.elf output/all.elf