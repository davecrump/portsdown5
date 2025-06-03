#!/usr/bin/bash

BASEDIR=$(pwd)
cd $BASEDIR

GAIN=95
echo $GAIN
GAIN=0"$(echo $GAIN/100 | bc -l | cut -c 1-3)"
echo $GAIN

exit
