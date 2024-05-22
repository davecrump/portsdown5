#! /bin/bash

# Compile LimeSDR BandViewer

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null

cd /home/pi/portsdown/src/limeview

touch limeview.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/limeview/limeview /home/pi/portsdown/bin/limeview

/home/pi/portsdown/bin/limeview


