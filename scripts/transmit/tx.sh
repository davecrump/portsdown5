#!/bin/bash

sudo rm videots >/dev/null 2>/dev/null
mkfifo videots

/home/pi/portsdown/bin/limesdr_dvb -i videots -s 1000000 -f 2/3 -r 1 -m DVBS2 -c QPSK -t 437e6 -g 0.82 -q 1 -D 27 -e 2&

echo starting netcat

netcat -u -4 -l 10000 > videots &
