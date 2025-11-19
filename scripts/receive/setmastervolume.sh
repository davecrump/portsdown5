#!/bin/bash

# Called with a parameter in the range 0 to 100.
# Sets the alsamixer volume.  Does not work for HDMI audio

MASTERVOLUME=$1

# Error check parameter
if [ "$MASTERVOLUME" -lt 0 ]; then
  MASTERVOLUME=0
fi
if [ "$MASTERVOLUME" -gt 100 ]; then
  MASTERVOLUME=100
fi

# echo Setting master volume $1

# Set it here for all the devices that we know about
# Some will generate harmless errors

amixer -c 2  sset Speaker $1% >/dev/null 2>/dev/null
amixer -c 3  sset Speaker $1% >/dev/null 2>/dev/null
amixer -c 2  sset PCM $1% >/dev/null 2>/dev/null
amixer -c 3  sset PCM $1% >/dev/null 2>/dev/null


exit



