#!/bin/sh

if [ ! -d llvm-8.0.0.src ]; then
	curl -L -O http://llvm.org/releases/8.0.0/llvm-8.0.0.src.tar.xz
	tar xvf llvm-8.0.0.src.tar.xz
	rm llvm-8.0.0.src.tar.xz
fi

if [ ! -d llvm-8.0.0.src/tools/clang ]; then
	curl -L -O http://llvm.org/releases/8.0.0/cfe-8.0.0.src.tar.xz
	tar xvf cfe-8.0.0.src.tar.xz
	mv cfe-8.0.0.src llvm-8.0.0.src/tools/clang
	rm cfe-8.0.0.src.tar.xz
fi

# Install ninja
brew install ninja

mkdir -p llvm-8.0.0.obj
cd llvm-8.0.0.obj
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV" ../llvm-8.0.0.src 
#make -j8 
cmake --build .
cd ..

echo "export LLVM_SRC=$PWD/llvm-8.0.0.src" > .envrc
echo "export LLVM_OBJ=$PWD/llvm-8.0.0.obj" >> .envrc
echo "export LLVM_DIR=$PWD/llvm-8.0.0.obj" >> .envrc
echo "PATH_add $PWD/llvm-8.0.0.obj/bin" >> .envrc

direnv allow .
