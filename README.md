# glitch_please_defense
This contains all the defenses (compiler based) for the Glitch Please project.
## Dependencies
* Clang/LLVM 8.0.0
### Installing LLVM/Clang
> Building LLVM and Clang from sources (This needs ~340GB disk space and compilation time of ~90min)

1) Download [LLVM-8.0.0](http://llvm.org/releases/8.0.0/llvm-8.0.0.src.tar.xz), [clang-8.0.0](http://llvm.org/releases/8.0.0/cfe-8.0.0.src.tar.xz)

2) Unzip the LLVM and Clang source files
```
tar xf llvm-8.0.0.src.tar.xz
tar xf cfe-8.0.0.src.tar.xz
mv cfe-8.0.0.src llvm-8.0.0.src/tools/clang
```

3) Create your target build folder and make
```
mkdir llvm-8.0.0.obj
cd llvm-8.0.0.obj
cmake -DCMAKE_BUILD_TYPE=Debug ../llvm-8.0.0.src (or add "-DCMAKE_BUILD_TYPE:STRING=Release" for releae version)
make -j8  
```

4) Add paths for LLVM and Clang
```
export LLVM_SRC=your_path_to_llvm-8.0.0.src
export LLVM_OBJ=your_path_to_llvm-8.0.0.obj
export LLVM_DIR=your_path_to_llvm-8.0.0.obj
export PATH=$LLVM_DIR/bin:$PATH
```
## Defenses
There are two main components of our defense:
* Source Rewriter (*gp_source_rewriter*)
* Instrumenter
## Source Rewriter (*gp_source_rewriter*)
As the name implies, this component uses source rewriting to apply the appropriate defenses. For this, we use `clang` tooling infrastructure.

More details of this could be found at: [gp_source_rewriter](https://github.com/Machiry/glitch_please_defense/tree/master/gp_source_rewriter)

## Instrumenter
This component uses the classic compiler based instrumentation to apply the defenses.

More details of this could be found at:
[Instrumenter](https://github.com/Machiry/glitch_please_defense/tree/master/instrumenter)
