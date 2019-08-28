# LLVM Passes
This folder contains various llvm passes.
## Building
Building all the passes.

    mkdir obj
    cmake ..
    make -j4

All the shared objects for the passes will be present inside the `obj` folder.

## Passes
   ### DelayInjector

  This pass inserts calls to function that introduces delay into the program to reduce the effectiveness of glitching. 
        
   > Usage:

```
   cd obj/DelayInjector
   opt -load ./libDelayInjector.so -injectDelay <input_bc_file> -o <path_to_output_bc_file>
 ```

  Make sure that you link: `instrumentation_snippets/delayfunc.c` when you build the final executable, as it contains the function `gpdelay` that implements the delay.
  
### IntegrityProtectorPass

  This pass protects the provided global variables by inserting integrity checks during its reads and writes.
To use this pass, you need to first identify the names of **global variables that need to be protected** and save them in a file. lets call this: `to_protect_globals.txt`.

Example contents of the file:
```
gpprotect_struc
gpprotect_arr
```


   > Usage:

```
   cd obj/IntegrityProtectorPass
   opt -load ./libIntegrityProtector.so -gpIntegrity -globals <path_to_to_protect_globals.txt> <input_bc_file> -o <path_to_output_bc_file>
 ```
Try to run it on examples: `examples/instrumenter/integrity.c` and `examples/instrumenter/integrity_arr.c`
