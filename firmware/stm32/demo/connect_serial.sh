#!/bin/sh
port=` ls /dev/tty.usb*`
screen $port 38400
