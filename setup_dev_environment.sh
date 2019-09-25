#!/bin/bash

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac
echo "Installing on ${machine}..."


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

# Link our source rewriter
pushd llvm-8.0.0.src/tools/clang/tools/
ln -sf ../../../../gp_source_rewriter .
popd

# Add it to CMake
if ! grep -q "gp_source_rewriter" llvm-8.0.0.src/tools/clang/tools/CMakeLists.txt; then
    if [ "$machine" = "Mac" ]; then
        sed -i '' '/add_clang_subdirectory(clang-format)/ a\
add_clang_subdirectory(gp_source_rewriter)\
' llvm-8.0.0.src/tools/clang/tools/CMakeLists.txt
    elif [ "$machine" = "Linux" ]; then
        sed -i '/add_clang_subdirectory(clang-format)/ a\
add_clang_subdirectory(gp_source_rewriter)\
' llvm-8.0.0.src/tools/clang/tools/CMakeLists.txt
    fi

fi


# Install ninja
if [ "$machine" = "Mac" ]; then
    brew install ninja || (echo "Could not install ninja" && exit 0)
elif [ "$machine" = "Linux" ]; then
    sudo apt-get -y install ninja-build || (echo "Could not install ninja" && exit 0)
fi


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


