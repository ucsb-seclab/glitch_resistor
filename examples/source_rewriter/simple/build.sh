#!/bin/sh
../../../build_source_rewriter.sh 
gp-source-rewriter --verbose -output-postfix gr *.c

gp-source-rewriter --verbose --allinit -output-postfix gr *.c

