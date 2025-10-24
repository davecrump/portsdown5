#! /bin/bash

# Compile each app in term checking for successful compile.
# Do not run the apps
# Use after a change to the common library

# Compiles:

# sa_sched.c
# portsdown5.c
# siggen5.c
# limeview.c
# nfmeter.c
# noise_meter.c
# dmm.c
# power_meter.c
# sweeper.c
# wav2lime.c
# fb2png.c
# msgbox.c
# picoview.c
# sdrplayview.c

# limeng bv - to do
# limeng nf - to do
# wav2limeng - to do

# sa_bv.c
# sa_if.c
# sa_sdr.c 

# set -x

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


# Compile the Portsdown app
cd /home/pi/portsdown/src/portsdown
touch portsdown5.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The Portsdown app failed to compile"
  echo "/home/pi/portsdown/src/portsdown/portsdown5.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Portsdown app compile"
  echo
  mv /home/pi/portsdown/src/portsdown/portsdown5 /home/pi/portsdown/bin/portsdown5
fi
cd /home/pi


# Compile the Signal Generator
cd /home/pi/portsdown/src/siggen
touch siggen5.c
make -j 4 -O
if [ $? != "0" ]; then
  echo "The Signal Generator failed to compile"
  echo "/home/pi/portsdown/src/siggen/siggen5.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Signal Generator compile"
  echo
  sudo make install
fi
cd /home/pi


# Compile the LimeSDR BandViewer
cd /home/pi/portsdown/src/limeview
touch limeview.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The Lime BandViewer failed to compile"
  echo "/home/pi/portsdown/src/limeview/limeview.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Lime BandViewer compile"
  echo
mv /home/pi/portsdown/src/limeview/limeview /home/pi/portsdown/bin/limeview
fi
cd /home/pi


# Compile the LimeSDR Noise Figure Meter
cd /home/pi/portsdown/src/nf_meter
touch nf_meter.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The Lime NF Meter failed to compile"
  echo "/home/pi/portsdown/src/nf_meter/nf_meter.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Lime NF Meter compile"
  echo
  mv /home/pi/portsdown/src/nf_meter/nf_meter /home/pi/portsdown/bin/nf_meter
fi


# Compile the LimeSDR Noise Meter
cd /home/pi/portsdown/src/noise_meter
touch noise_meter.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The Lime Noise Meter failed to compile"
  echo "/home/pi/portsdown/src/noise_meter/noise_meter.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Lime Noise Meter compile"
  echo
  mv /home/pi/portsdown/src/noise_meter/noise_meter /home/pi/portsdown/bin/noise_meter
fi
cd /home/pi


# Compile the DMM Display
cd /home/pi/portsdown/src/dmm
touch dmm.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The DMM Display failed to compile"
  echo "/home/pi/portsdown/src/dmm/dmm.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful DMM Display compile"
  echo
  mv /home/pi/portsdown/src/dmm/dmm /home/pi/portsdown/bin/dmm
fi
cd /home/pi


# Compile the Power Meter
cd /home/pi/portsdown/src/power_meter
touch power_meter.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The Power Meter failed to compile"
  echo "/home/pi/portsdown/src/power_meter/power_meter.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Power Meter compile"
  echo
  mv /home/pi/portsdown/src/power_meter/power_meter /home/pi/portsdown/bin/power_meter
fi
cd /home/pi


# Compile the Sweeper
cd /home/pi/portsdown/src/sweeper
touch sweeper.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The Sweeper failed to compile"
  echo "/home/pi/portsdown/src/sweeper/sweeper.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful Sweeper compile"
  echo
  mv /home/pi/portsdown/src/sweeper/sweeper /home/pi/portsdown/bin/sweeper
fi
cd /home/pi


# Compile the fb2png utility
cd /home/pi/portsdown/src/fb2png
touch fb2png.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The fb2png utility failed to compile"
  echo "/home/pi/portsdown/src/fb2png/fb2png.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful fb2png compile"
  echo
  mv /home/pi/portsdown/src/fb2png/fb2png /home/pi/portsdown/bin/fb2png
fi
cd /home/pi


# Compile the msgbox utility
cd /home/pi/portsdown/src/msgbox
touch msgbox.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The msgbox utility failed to compile"
  echo "/home/pi/portsdown/src/msgbox/msgbox.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful msgbox compile"
  echo
  mv /home/pi/portsdown/src/msgbox/msgbox /home/pi/portsdown/bin/msgbox
fi
cd /home/pi


# Compile PicoViewer
cd /home/pi/portsdown/src/picoview
touch picoview.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The PicoViewer failed to compile"
  echo "/home/pi/portsdown/src/picoview/picoview.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful PicoViewer compile"
  echo
  mv /home/pi/portsdown/src/picoview/picoview /home/pi/portsdown/bin/picoview
fi
cd /home/pi


# Compile the wav2lime utility
cd /home/pi/portsdown/src/wav2lime
touch wav2lime.c
gcc -o wav2lime wav2lime.c -lLimeSuite
if [ $? != "0" ]; then
  echo
  echo "wav2lime failed to compile"
  echo "/home/pi/portsdown/src/wav2lime/wav2lime.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful wav2lime compile"
  echo
  mv /home/pi/portsdown/src/wav2lime/wav2lime /home/pi/portsdown/bin/wav2lime
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
cd /home/pi/portsdown/src/sa_if
touch sa_if.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "The SA AM IF failed to compile"
  echo "/home/pi/portsdown/src/sa_if/sa_if.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA AM IF compile"
  echo
  mv /home/pi/portsdown/src/sa_if/sa_if /home/pi/portsdown/bin/sa_if
fi
cd /home/pi


# Compile the SA wideband SDR
cd /home/pi/portsdown/src/sa_sdr
touch sa_sdr.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The SA wideband SDR failed to compile"
  echo "/home/pi/portsdown/src/sa_if/sa_if.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SA wideband SDR compile"
  echo
  mv /home/pi/portsdown/src/sa_sdr/sa_sdr /home/pi/portsdown/bin/sa_sdr
fi
cd /home/pi


# Compile the SDRPlay BandViewer
cd /home/pi/portsdown/src/sdrplayview
touch sdrplayview.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "The SDRPlay BandViewer failed to compile"
  echo "/home/pi/portsdown/src/sdrplayview/sdrplayview.c"
  cd /home/pi
  exit
else
  echo
  echo "Successful SDRPlay BandViewer compile"
  echo
  mv /home/pi/portsdown/src/sdrplayview/sdrplayview /home/pi/portsdown/bin/sdrplayview
fi
cd /home/pi


echo "All apps successfuly compiled"
exit


