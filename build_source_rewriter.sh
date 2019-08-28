#!/bin/sh

cp -r gp_source_rewriter llvm-8.0.0.src/tools/clang/tools/

if ! grep -q "gp_source_rewriter" llvm-8.0.0.src/tools/clang/tools/CMakeLists.txt; then
	sed -i '' '/add_clang_subdirectory(clang-format)/ a\
add_clang_subdirectory(gp_source_rewriter)\
' llvm-8.0.0.src/tools/clang/tools/CMakeLists.txt
fi

cd llvm-8.0.0.obj
#cmake -DCMAKE_BUILD_TYPE=Debug ../llvm-8.0.0.src
#make gp-source-rewriter

cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="RISCV" ../llvm-8.0.0.src 
cmake --build . --target gp-source-rewriter
