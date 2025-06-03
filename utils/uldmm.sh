#! /bin/bash

# Compile dmm

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null
sudo killall sa_if >/dev/null 2>/dev/null
sudo killall sa_bv >/dev/null 2>/dev/null
sudo killall dmm >/dev/null 2>/dev/null

echo Compiling dmm.c

cd /home/pi/portsdown/src/dmm

touch dmm.c

make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/dmm/dmm /home/pi/portsdown/bin/dmm

/home/pi/portsdown/bin/dmm


