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
make gp-source-rewriter
```
## Running the tool
We run the tool the same way as we run any compiler i.e., `gcc` or `clang`
```
cd <LLVM_OBJ>/bin
USAGE: gp-source-rewriter [options] <source0> [... <sourceN>]

OPTIONS:

Generic Options:

  -help                      - Display available options (-help-hidden for more)
  -help-list                 - Display list of available options (-help-list-hidden for more)
  -version                   - Display the version of this program

gp-source-rewriter options:

  -allinit                   - Replace an enum only if none of the enum values have initializers.
  -base-dir=<string>         - Base directory for the code we're translating
  -extra-arg=<string>        - Additional argument to append to the compiler command line
  -extra-arg-before=<string> - Additional argument to prepend to the compiler command line
  -output-postfix=<string>   - Postfix to add to the names of rewritten files, if not supplied writes to STDOUT
  -p=<string>                - Build path
  -verbose                   - Print verbose information

-p <build-path> is used to read a compile command database.

        For example, it can be a CMake build directory in which a file named
        compile_commands.json exists (use -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        CMake option to get this output). When no build path is specified,
        a search for compile_commands.json will be attempted through all
        parent paths of the first input file . See:
        http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html for an
        example of setting up Clang Tooling on a source tree.

<source0> ... specify the paths of source files. These paths are
        looked up in the compile command database. If the path of a file is
        absolute, it needs to point into CMake's source tree. If the path is
        relative, the current working directory needs to be in the CMake
        source tree and the file must be in a subdirectory of the current
        working directory. "./" prefixes in the relative files will be
        automatically removed, but the rest of a relative path must be a
        suffix of a path in the compile command database.

```

### Example:
```
$ gp-source-rewriter -output-postfix=glitch simple.c
# this will create the file simple.glitch.c which contains enums in simple.c replaced with hamming constants.
```

### TODO:
* Instructions to run a tool on a project.
