#! /bin/bash

# Compile sa_if

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null
sudo killall sa_if >/dev/null 2>/dev/null

echo Compiling sa_if.c

cd /home/pi/portsdown/src/sa_if

touch sa_if.c

make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sa_if/sa_if /home/pi/portsdown/bin/sa_if

/home/pi/portsdown/bin/sa_if


