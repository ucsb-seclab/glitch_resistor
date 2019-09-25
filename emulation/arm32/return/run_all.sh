#!/bin/sh
for file in *.s
do
#  gr-bitflipper $file &
  gr-bitflipper $file -F 2 -p 64 &
done
