#!/bin/sh

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac
echo "Installing on ${machine}..."


mkdir -p runtime
cd runtime

if [ "$machine" = "Mac" ]; then
    ARM_GCC="https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-mac.tar.bz2?revision=6a06dd2b-bb98-4708-adac-f4c630c33f4f?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Mac%20OS%20X,8-2019-q3-update"
    RISCV_GCC="https://static.dev.sifive.com/dev-tools/riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-apple-darwin.tar.gz"
    RISCV_OPENOCD="https://static.dev.sifive.com/dev-tools/riscv-openocd-0.10.0-2019.05.1-x86_64-apple-darwin.tar.gz"
elif [ "$machine" = "Linux" ]; then
    ARM_GCC="https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2?revision=c34d758a-be0c-476e-a2de-af8c6e16a8a2?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2019-q3-update"
    RISCV_GCC="https://static.dev.sifive.com/dev-tools/riscv64-unknown-elf-gcc-8.2.0-2019.05.3-x86_64-linux-ubuntu14.tar.gz"
    RISCV_OPENOCD="https://static.dev.sifive.com/dev-tools/riscv-openocd-0.10.0-2019.05.1-x86_64-linux-ubuntu14.tar.gz"
fi

    # ARM
    if [ ! -d gcc-arm-none-eabi-* ]; then
            #curl -L -O https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
            #tar -xvf gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
            #rm gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
        curl -L "$ARM_GCC" > gcc-arm-none-eabi.tar.bz2
        tar -xvf gcc-arm-none-eabi.tar.bz2
        rm gcc-arm-none-eabi.tar.bz2
    fi
    # RISC-V
    if [ ! -d riscv64-unknown-elf-gcc-* ]; then
            curl -L -O "$RISCV_GCC"
            tar -xvf riscv64-unknown-elf-gcc-*.tar.gz
            rm riscv64-unknown-elf-gcc-*.tar.gz
            mv riscv64-unknown-elf-gcc-* riscv64-unknown-elf-gcc-8.2.0-2019.05.3
    fi
    # RISC-V OpenOCD
    if [ ! -d riscv-openocd-* ]; then
        curl -L -O "$RISCV_OPENOCD"
        tar -xvf riscv-openocd-*.tar.gz
        rm riscv-openocd-*.tar.gz
        mv riscv-openocd-* riscv-openocd-0.10.0-2019.05.1
    fi

cd ..

echo "export ARM_GCC_TOOLCHAIN=$PWD/runtime/gcc-arm-none-eabi-8-2019-q3-update/bin" >> .envrc
echo "export RISCV_OPENOCD_PATH=$PWD/runtime/riscv-openocd-0.10.0-2019.05.1" >> .envrc
echo "export RISCV_PATH=$PWD/runtime/riscv64-unknown-elf-gcc-8.2.0-2019.05.3" >> .envrc
echo "PATH_add $PWD/runtime/gcc-arm-none-eabi-8-2019-q3-update/bin" >> .envrc
direnv allow .

