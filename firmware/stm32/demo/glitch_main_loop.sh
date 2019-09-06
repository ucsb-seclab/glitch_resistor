#!/bin/sh
arm-none-eabi-gdb -x glitch_main_loop.txt build/test_firmware.elf
