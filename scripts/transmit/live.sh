#!/bin/bash

sudo rm videots
mkfifo videots

netcat -u -4 -l 10000 > videots &


/home/pi/portsdown/bin/DvbTsToIQ/dvb2iq -i videots -s 1000 -f 2/3 -r 4 -m DVBS | /home/pi/portsdown/bin/limesdr_send -f 437e6 -b 2.5e6 -s 1000000 -g 0.7 -p 0.05 -a BAND2 -r 4 -l 102400


# vlc videots

#cvlc udp://:@:10000