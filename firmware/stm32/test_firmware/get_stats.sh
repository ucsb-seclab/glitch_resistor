#!/bin/bash

echo "Instrumented elements:"
passes="Timing Branch Loop Ret"
for p in $passes; 
do
	grep "Instrumenting" output/all.txt | grep "GR/$p" | wc | awk '{print $1;}' | xargs echo "$p:	$1"
done

echo ""
echo "Negated:"
for p in $passes;
do
        grep "Negated." output/all.txt | grep "GR/$p" | wc | awk '{print $1;}' | xargs echo "$p:	$1"
done

echo ""
echo "Sizes:"
files="none branch delay integrity loop ret all_nodelay all"
awk '/text/{print}' output/all.txt
for p in $files;
do
	awk '/text/{getline; print}' output/$p.txt
done

echo ""
echo "Lines of code:"
cloc --csv --csv-delimiter=$'\t' Src/ Drivers/ Inc/ startup/
