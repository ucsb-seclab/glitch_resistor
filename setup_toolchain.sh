#!/bin/sh

if [ ! -d runtime/gcc-arm-none-eabi-5_4-2016q3 ]; then
	mkdir -p runtime
	cd runtime
	curl -L -O https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
	tar -xvf gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
	rm gcc-arm-none-eabi-5_4-2016q3-20160926-mac.tar.bz2
	cd ..
fi

echo "export ARM_GCC_TOOLCHAIN=$PWD/runtime/gcc-arm-none-eabi-5_4-2016q3/bin" >> .envrc
echo "PATH_add $PWD/runtime/gcc-arm-none-eabi-5_4-2016q3/bin" >> .envrc
direnv allow .

