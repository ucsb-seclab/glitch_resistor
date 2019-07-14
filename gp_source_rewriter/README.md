# gp_source_rewriter
This is the source code rewriter that implements certain defenses against glitching.

**Make sure that you have setup LLVM and clang by following steps [here](https://github.com/Machiry/glitch_please_defense#installing-llvmclang)**
## Building
1. Copy sources to `clang` tools directory:
```
cp -r gp_source_rewriter <LLVM_SRC>/tools/clang/tools
```
2. Modify the `<LLVM_SRC>/tools/clang/tools/CMakeLists.txt`
Here, we need to add `gp_source_rewriter` to clang tools.
```diff
add_clang_subdirectory(clang-format)
+ add_clang_subdirectory(gp_source_rewriter)
add_clang_subdirectory(clang-format-vs)
```
3. Build
```
cd <LLVM_OBJ>
make gp_source_rewriter
```
## Running the tool
We run the tool the same way as we run any compiler i.e., `gcc` or `clang`
```
cd <LLVM_OBJ>/bin
./gp_source_rewriter [COMPILER ARGS] [SOURCE FILES]
```
