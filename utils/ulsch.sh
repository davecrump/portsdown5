#! /bin/bash

# Compile sa_sched

# set -x

sudo killall portsdown5 >/dev/null 2>/dev/null
sudo killall limeview >/dev/null 2>/dev/null
sudo killall sa_if >/dev/null 2>/dev/null
sudo killall sa_sched >/dev/null 2>/dev/null

echo Compiling sa_sched.c

cd /home/pi/portsdown/src/sa_sched

touch sa_sched.c

make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi

cd /home/pi

mv /home/pi/portsdown/src/sa_sched/sa_sched /home/pi/portsdown/bin/sa_sched

/home/pi/portsdown/bin/sa_sched
