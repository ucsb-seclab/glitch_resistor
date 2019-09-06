#!/bin/bash

echo "Instrumented elements:"
passes="Timing Branch Loop Ret"
for p in $passes; 
do
	grep "Instrumenting" output/all.txt | grep "GR/$p" | wc | awk '{print $1;}' | xargs echo "$p:$1"
done

echo "Sizes:"
files="none branch delay integrity loop ret all"
awk '/text/{print}' output/all.txt
for p in $files;
do
	awk '/text/{getline; print}' output/$p.txt
done

echo "Lines of code:"
cloc --csv --csv-delimiter=$'\t' Src/ Drivers/ Inc/ startup/
