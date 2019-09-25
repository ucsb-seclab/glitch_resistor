#!/bin/sh
for file in *.S
do
  gr-bitflipper $file -F 1
  gr-bitflipper $file -F 2

  gr-bitflipper $file -F 1 --force_invalid -O ./results_invalid
  gr-bitflipper $file -F 2 --force_invalid -O ./results_invalid

done
