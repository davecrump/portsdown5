#!/bin/bash

sudo killall limesdr_dvb
sudo killall limesdr_dvbng

sudo killall netcat

sudo killall tx.sh

/home/pi/portsdown/bin/limesdr_stopchannel

exit