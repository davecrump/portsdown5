#! /bin/bash

# Stop all running apps

# set -x

if pgrep -x "startup.sh" >/dev/null
then
  ps -ef | grep startup.sh | grep -v grep | awk '{print $2}' | xargs kill
fi

# Portsdown processes
sudo killall portsdown5 > /dev/null 2>/dev/null
sudo killall netcat > /dev/null 2>/dev/null
sudo killall dvb2iq > /dev/null 2>/dev/null

# Portsdown Test Equipment Processes
sudo killall limeview > /dev/null 2>/dev/null
sudo killall limeviewng > /dev/null 2>/dev/null
sudo killall siggen > /dev/null 2>/dev/null
sudo killall nf_meter > /dev/null 2>/dev/null
sudo killall sweeper > /dev/null 2>/dev/null
sudo killall power_meter > /dev/null 2>/dev/null
sudo killall noise_meter > /dev/null 2>/dev/null
sudo killall dmm > /dev/null 2>/dev/null

# KeyLimePi Analyser processes
sudo killall sa_if > /dev/null 2>/dev/null
sudo killall sa_bv > /dev/null 2>/dev/null
sudo killall sa_sdr > /dev/null 2>/dev/null
sudo killall sa_sched > /dev/null 2>/dev/null

# Transmit processes
sudo killall ffmpeg > /dev/null 2>/dev/null
sudo killall limesdr_dvb > /dev/null 2>/dev/null
sudo killall netcat > /dev/null 2>/dev/null

# Receive processes
sudo killall vlc > /dev/null 2>/dev/null
sudo killall ffplay > /dev/null 2>/dev/null



