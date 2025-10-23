#! /bin/bash

# Compile SDRPlay BandViewer

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall sdrplayview >/dev/null 2>/dev/null

echo Compiling sdrplayview.c

cd /home/pi/portsdown/src/sdrplayview

touch sdrplayview.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sdrplayview/sdrplayview /home/pi/portsdown/bin/sdrplayview

/home/pi/portsdown/bin/sdrplayview


