#!/bin/sh

cat > 99-newae.rules <<RULES
# CW-Lite
SUBSYSTEM=="usb", ATTRS{idVendor}=="2b3e", ATTRS{idProduct}=="ace2", MODE="0664", GROUP="plugdev"

# CW-1200
SUBSYSTEM=="usb", ATTRS{idVendor}=="2b3e", ATTRS{idProduct}=="ace3", MODE="0664", GROUP="plugdev"

# CW-305 (Artix Target)
SUBSYSTEM=="usb", ATTRS{idVendor}=="2b3e", ATTRS{idProduct}=="c305", MODE="0664", GROUP="plugdev"

# CW-CR2
SUBSYSTEM=="usb", ATTRS{idVendor}=="04b4", ATTRS{idProduct}=="8613", MODE="0664", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="221a", ATTRS{idProduct}=="0100", MODE="0664", GROUP="plugdev"
RULES

sudo mv 99-newae.rules /etc/udev/rules.d/99-newae.rules

sudo usermod -a -G plugdev `whoami`
sudo udevadm control --reload-rules
