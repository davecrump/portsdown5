#! /bin/bash

# Reload LimeSuiteNG from GitHub

set -x

sudo rm -rf /home/pi/LimeSuiteNG/

cd /home/pi

git clone https://github.com/myriadrf/LimeSuiteNG        # Download LimeSuiteNG

cd LimeSuiteNG

cmake -B build

cd build

make -j 4 -O

sudo make install

sudo ldconfig


