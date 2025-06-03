#! /bin/bash

sudo killall wav2limeng
/home/pi/portsdown/bin/limesdr_stopchannel

cd /home/pi/portsdown/src/wav2limeng
gcc -o wav2limeng wav2limeng.c -llimesuiteng
cp /home/pi/portsdown/src/wav2limeng/wav2limeng /home/pi/portsdown/bin/wav2limeng
cd /home/pi

/home/pi/portsdown/bin/wav2limeng -i /home/pi/hamtv_short.wav -g 0.6 -f 437