#! /bin/bash

sudo killall siggen >/dev/null 2>/dev/null
sudo killall portsdown >/dev/null 2>/dev/null

# Compile Signal Generator
cd /home/pi/portsdown/src/siggen
touch siggen5.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
sudo make install

cd /home/pi
reset

/home/pi/portsdown/bin/siggen 1

