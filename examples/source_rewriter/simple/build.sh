#!/bin/sh
../../../build_source_rewriter.sh 
gp-source-rewriter --verbose -output-postfix bak *.c

gp-source-rewriter --verbose --allinit -output-postfix bak2 *.c

