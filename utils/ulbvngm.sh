#! /bin/bash

# Compile LimeSDR BandViewer NG version for LimeSDR Micro

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeviewng >/dev/null 2>/dev/null

echo Compiling limeviewng.c

cd /home/pi/portsdown/src/limeviewngm

touch limeviewng.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/limeviewngm/limeviewng /home/pi/portsdown/bin/limeviewng

/home/pi/portsdown/bin/limeviewng


