#!/bin/bash

sudo killall limesdr_dvb
sudo killall limesdr_dvbng
sudo killall ffmpeg

sudo killall netcat

sudo killall tx.sh
sudo killall tx_rf.sh
sudo killall tx_video.sh

sleep 1

/home/pi/portsdown/bin/limesdr_stopchannel
sudo killall -9 netcat
exit