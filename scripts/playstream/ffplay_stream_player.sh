#!/bin/bash

# Called by streamrx to start the stream receiver

set -x

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"
RCONFIGFILE="/home/pi/portsdown/configs/longmynd_config.txt"
PRESETFILE="/home/pi/portsdown/configs/stream_presets.txt"

############ FUNCTION TO READ CONFIG FILE #############################

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}
########################################################################

cd /home/pi

# Read from receiver config file
#AUDIO_OUT=$(get_config_var audio $RCONFIGFILE)
#VLCVOLUME=$(get_config_var vlcvolume $PCONFIGFILE)
AUDIO_OUT=usb
VLCVOLUME=128

VLCTRANSFORM=$(get_config_var vlctransform $SCONFIGFILE)
STREAMNUMBER=$(get_config_var selectedstream $PRESETFILE)

STREAMREF=stream"$STREAMNUMBER"

# echo $STREAMREF

STREAMURL=$(get_config_var $STREAMREF $PRESETFILE)

# Read in URL argument
#STREAMURL=$1
#STREAMURL=rtmp://rtmp.batc.org.uk/live/gb3ey

# Send audio to the correct port
if [ "$AUDIO_OUT" == "rpi" ]; then                  # Portsdown 4 3.5mm jack
  # Check for Raspberry Pi 4
  aplay -l | grep -q 'bcm2835 Headphones'
  if [ $? == 0 ]; then
    AUDIO_DEVICE="hw:CARD=Headphones,DEV=0"
  else # Raspberry Pi 5                             # No audio out on RPi 5
    AUDIO_DEVICE=" "
  fi
elif [ "$AUDIO_OUT" == "usb" ]; then                # USB Dongle
  AUDIO_DEVICE="hw:CARD=Device,DEV=0"
elif [ "$AUDIO_OUT" == "hdmi" ]; then               # HDMI
  AUDIO_DEVICE="default"
else                                                # Custom
  AUDIO_DEVICE=hw:CARD="$AUDIO_OUT",DEV=0
fi

# Error check volume
if [ "$VLCVOLUME" -lt 0 ]; then
  VLCVOLUME=0
fi
if [ "$VLCVOLUME" -gt 512 ]; then
  VLCVOLUME=512
fi

  # Sort display rotation
  if [ "$VLCTRANSFORM" == "0" ]; then
    VF=""
    VFROTATE=""
  elif [ "$VLCTRANSFORM" == "90" ]; then
    VF="-vf"
    VFROTATE="transpose=1"
  elif [ "$VLCTRANSFORM" == "180" ]; then
    VF="-vf"
    VFROTATE="transpose=1,transpose=1"
  elif [ "$VLCTRANSFORM" == "270" ]; then
    VF="-vf"
    VFROTATE="transpose=2"
  fi


sudo killall ffplay >/dev/null 2>/dev/null

SDL_AUDIODRIVER="alsa" AUDIODEV="$AUDIO_DEVICE" ffplay $VF $VFROTATE $STREAMURL &


exit


