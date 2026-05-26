#! /bin/bash

# Compile each KLP SA app in term checking for successful compile.
# Do not run the apps
# Use after a change to the common library

# Compiles:

# sa_sched.c
# sa_bv.c
# sa_if_wb.c
# sa_sdr.c


# Make sure that nothing is running
/home/pi/portsdown/utils/stop.sh

# Compile the SA Scheduler
cd /home/pi/portsdown/src/sa_sched
touch sa_sched.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The SA Scheduler failed to compile"
  echo "/home/pi/portsdown/src/sa_sched/sa_sched.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA Scheduler compile"
  echo
  mv /home/pi/portsdown/src/sa_sched/sa_sched /home/pi/portsdown/bin/sa_sched
fi
cd /home/pi


# Compile the SA IF BandViewer
cd /home/pi/portsdown/src/sa_bv
touch sa_bv.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The SA IF BandViewer failed to compile"
  echo "/home/pi/portsdown/src/sa_bv/sa_bv.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA IF BandViewer compile"
  echo
  mv /home/pi/portsdown/src/sa_bv/sa_bv /home/pi/portsdown/bin/sa_bv
fi
cd /home/pi


# Compile the SA AM IF
cd /home/pi/portsdown/src/sa_if_wb
touch sa_if.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The SA AM IF failed to compile"
  echo "/home/pi/portsdown/src/sa_if_wb/sa_if.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA AM IF compile"
  echo
  mv /home/pi/portsdown/src/sa_if_wb/sa_if /home/pi/portsdown/bin/sa_if
fi
cd /home/pi


# Compile the SA raw SDR
cd /home/pi/portsdown/src/sa_sdr
touch sa_sdr.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The SA wideband SDR failed to compile"
  echo "/home/pi/portsdown/src/sa_sdr/sa_sdr.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA wideband SDR compile"
  echo
  mv /home/pi/portsdown/src/sa_sdr/sa_sdr /home/pi/portsdown/bin/sa_sdr
fi
cd /home/pi
