#! /bin/bash

# Reload LimeSuiteNG for LimeSDR Micro from GitHub

set -x

sudo rm -rf /home/pi/LimeSuiteNG/

cd /home/pi

git clone -b limesdr-micro https://github.com/myriadrf/LimeSuiteNG        # Download LimeSuiteNG

cd LimeSuiteNG

sudo ./install_dependencies.sh

cmake -B build

cd build

make -j 4 -O

sudo make install

sudo ldconfig

# To temporarily go back to a previous commit, run git checkout 626a781f2f1d20e25b3e731f3fe8fb350a7adc97

