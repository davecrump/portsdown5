#!/bin/bash

sudo killall limesdr_dvb
sudo killall limesdr_dvbng
sudo killall ffmpeg

sudo killall netcat

sudo killall tx.sh
sudo killall tx_rf.sh
sudo killall tx_video.sh

/home/pi/portsdown/bin/limesdr_stopchannel

exit