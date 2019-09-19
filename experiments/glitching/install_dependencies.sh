#!/bin/sh
#sudo apt-get install python2.7 python2.7-dev python2.7-libs python-numpy python-scipy python-pyside python-configobj python-setuptools python-pip
pip install pyusb
pip install pyqtgraph
git clone https://github.com/cspensky/chipwhisperer.git
cd chipwhisperer
python -m pip install -e .