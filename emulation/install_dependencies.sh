#!/bin/sh

# OSX?
if which brew > /dev/null
then
	brew install unicorn
	if ! which direnv > /dev/null
	then
		echo 'Direnv installation...'
		brew install direnv
	fi

	# Fix enum
	pip install enum34
else
	sudo apt-get -y install python-pip
	sudo apt-get -y install cmake
	sudo apt-get -y install direnv
	sudo apt-get -y install libssl-dev
fi

pip install unicorn
pip install capstone
git clone https://github.com/keystone-engine/keystone.git
cd keystone
mkdir -p build
cd build
../make-share.sh
sudo make install
cd .. # keystone
cd bindings
make
cd python
python setup.py install --verbose
cd .. # bindings
cd .. # keystone
cd .. # ./


echo "Patching up keystone paths..."
sudo ln -s /usr/local/lib/libkeystone.so /usr/local/lib/python2.7/dist-packages/keystone/

echo "export PYTHONPATH=$PWD" > .envrc
echo "export DYLD_LIBRARY_PATH=/usr/local/opt/unicorn/lib/:$DYLD_LIBRARY_PATH" >> .envrc
echo "PATH_add $PWD/bin" >> .envrc
direnv allow .


