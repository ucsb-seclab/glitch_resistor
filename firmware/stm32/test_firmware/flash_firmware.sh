#!/bin/sh
make
st-flash --format binary --flash=512k write build/test_firmware.bin 0x08000000
