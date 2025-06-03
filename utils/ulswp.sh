#! /bin/bash

# Compile sweeper

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null
sudo killall sa_if >/dev/null 2>/dev/null
sudo killall sa_bv >/dev/null 2>/dev/null
sudo killall nf_meter >/dev/null 2>/dev/null
sudo killall sweeper >/dev/null 2>/dev/null

echo Compiling sweeper.c

cd /home/pi/portsdown/src/sweeper

touch sweeper.c

make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sweeper/sweeper /home/pi/portsdown/bin/sweeper

/home/pi/portsdown/bin/sweeper


