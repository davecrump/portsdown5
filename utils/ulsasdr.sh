#! /bin/bash

# Compile LimeSDR BandViewer for SA

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null

echo Compiling sa_sdr.c

cd /home/pi/portsdown/src/sa_sdr

touch sa_sdr.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sa_sdr/sa_sdr /home/pi/portsdown/bin/sa_sdr

/home/pi/portsdown/bin/sa_sdr


