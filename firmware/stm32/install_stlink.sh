#!/bin/sh

sudo apt-get install libusb-1.0-0-dev


git clone https://github.com/texane/stlink.git
cd stlink
make release

pushd build/Release; sudo make install
popd

#install udev rules
sudo cp etc/udev/rules.d/49-stlinkv* /etc/udev/rules.d/
#and restart udev
sudo udevadm control --reload

# Just in case
sudo ldconfig  
