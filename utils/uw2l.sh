#! /bin/bash

sudo killall wav2lime
/home/pi/portsdown/bin/limesdr_stopchannel

cd /home/pi/portsdown/src/wav2lime
gcc -o wav2lime wav2lime.c -lLimeSuite
cp /home/pi/portsdown/src/wav2lime/wav2lime /home/pi/portsdown/bin/wav2lime
cd /home/pi

/home/pi/portsdown/bin/wav2lime -i /home/pi/hamtv_short.wav -g 0.6 -f 437