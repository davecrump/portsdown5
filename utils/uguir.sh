#! /bin/bash

# Compile portsdown gui

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
cd /home/pi/portsdown/src/gui
# make clean
touch portsdown5.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
#sudo make install
#cd ../

cd /home/pi
#reset

mv /home/pi/portsdown/src/gui/portsdown5 /home/pi/portsdown/bin/portsdown5

/home/pi/portsdown/bin/portsdown5



