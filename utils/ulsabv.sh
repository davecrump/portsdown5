#! /bin/bash

# Compile sa_bv

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null
sudo killall sa_if >/dev/null 2>/dev/null
sudo killall sa_bv >/dev/null 2>/dev/null

echo Compiling sa_bv.c

cd /home/pi/portsdown/src/sa_bv

touch sa_bv.c

make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sa_bv/sa_bv /home/pi/portsdown/bin/sa_bv

/home/pi/portsdown/bin/sa_bv


