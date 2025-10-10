#! /bin/bash

# Compile Pico A-D Viewer

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall picoview >/dev/null 2>/dev/null

echo Compiling picoview.c

cd /home/pi/portsdown/src/picoview

touch picoview.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/picoview/picoview /home/pi/portsdown/bin/picoview

/home/pi/portsdown/bin/picoview


