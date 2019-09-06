# Examples
This directory numerous bite-sized snippets of code to provide working examples of each of our defenses

*instrumenter*  contains examples of compile-time instrumentations
*source_rewriter contains examples of source-code rewriting

Every project is accompanied by a Makefile, which should build using your native compiler and a `build.sh` script which will build using *clang* and  the appropriate pass.
The output file will always be `example`

To see how to enable individual passes, see the appropriate `build.sh`

If the defense changes the file (e.g., the source code re-writing), a backup will made with the .bak extension.
