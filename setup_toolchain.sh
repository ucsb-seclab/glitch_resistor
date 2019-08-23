#!/bin/sh

mkdir -p runtime
cd runtime

# ARM
if [ ! -d gcc-arm-none-eabi-8-2019-q3-update ]; then
        #curl -L -O https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
        #tar -xvf gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
        #rm gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
	curl -L -O "https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-mac.tar.bz2?revision=6a06dd2b-bb98-4708-adac-f4c630c33f4f?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Mac%20OS%20X,8-2019-q3-update"
	tar -xzvf gcc-arm-none-eabi-*tar*
	rm gcc-arm-none-eabi-*tar*
fi
# RISC-V
if [ ! -d riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin ]; then
        curl -L -O https://static.dev.sifive.com/dev-tools/riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin.tar.gz
        tar -xvf riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin.tar.gz
        rm riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin.tar.gz
fi
# RISC-V OpenOCD
if [ ! -d riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin ]; then
	curl -L -O https://static.dev.sifive.com/dev-tools/riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin.tar.gz
	tar -xvf riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin.tar.gz
	rm riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin.tar.gz
fi


cd ..

echo "export ARM_GCC_TOOLCHAIN=$PWD/runtime/gcc-arm-none-eabi-8-2019-q3-update/bin" >> .envrc
echo "export RISCV_OPENOCD_PATH=$PWD/runtime/riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin" >> .envrc
echo "export RISCV_PATH=$PWD/runtime/riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin
" >> .envrc
echo "export LLVM_CC=clang" >> .envrc
echo "PATH_add $PWD/runtime/gcc-arm-none-eabi-8-2019-q3-update/bin" >> .envrc
direnv allow .

