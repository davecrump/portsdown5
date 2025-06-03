#!/bin/bash

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"
RCONFIGFILE="/home/pi/portsdown/configs/longmynd_config.txt"

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

cd /home/pi

# Read from receiver config file
SYMBOLRATEK=$(get_config_var sr0 $RCONFIGFILE)
SYMBOLRATEK_T=$(get_config_var sr1 $RCONFIGFILE)
FREQ_KHZ=$(get_config_var freq0 $RCONFIGFILE)
FREQ_KHZ_T=$(get_config_var freq1 $RCONFIGFILE)
RX_MODE=$(get_config_var mode $RCONFIGFILE)
Q_OFFSET=$(get_config_var qoffset $RCONFIGFILE)
AUDIO_OUT=$(get_config_var audio $RCONFIGFILE)
INPUT_SEL=$(get_config_var input $RCONFIGFILE)
INPUT_SEL_T=$(get_config_var input1 $RCONFIGFILE)
LNBVOLTS=$(get_config_var lnbvolts $RCONFIGFILE)
TSTIMEOUT=$(get_config_var tstimeout $RCONFIGFILE)
TSTIMEOUT_T=$(get_config_var tstimeout1 $RCONFIGFILE)
SCANWIDTH=$(get_config_var scanwidth $RCONFIGFILE)
SCANWIDTH_T=$(get_config_var scanwidth1 $RCONFIGFILE)
CHAN=$(get_config_var chan $RCONFIGFILE)
CHAN_T=$(get_config_var chan1 $RCONFIGFILE)
VLCVOLUME=$(get_config_var vlcvolume $PCONFIGFILE)
VLCTRANSFORM=$(get_config_var vlctransform $SCONFIGFILE)

# Correct for LNB LO Frequency if required
if [ "$RX_MODE" == "sat" ]; then

  # Use ffplay for the beacon
  if [ "$FREQ_KHZ" == "10491500" ]; then
    FFPLAY="yes"
  else
    FFPLAY="no"
  fi

  let FREQ_KHZ=$FREQ_KHZ-$Q_OFFSET
else
  FREQ_KHZ=$FREQ_KHZ_T
  SYMBOLRATEK=$SYMBOLRATEK_T
  INPUT_SEL=$INPUT_SEL_T
  TSTIMEOUT=$TSTIMEOUT_T
  SCANWIDTH=$SCANWIDTH_T
  CHAN=$CHAN_T
fi

# Send audio to the correct port
if [ "$AUDIO_OUT" == "rpi" ]; then             # RPi (non) jack
  AUDIO_DEVICE="hw:CARD=Headphones,DEV=0"
else                                           # USB Dongle
  AUDIO_DEVICE="hw:CARD=Device,DEV=0" 
fi
if [ "$AUDIO_OUT" == "hdmi" ]; then            # HDMI (not working)
  # Overide for HDMI
   AUDIO_DEVICE="hw:CARD=vc4hdmi0,DEV=0"
fi

# Select the correct tuner input
INPUT_CMD=" "
if [ "$INPUT_SEL" == "b" ]; then
  INPUT_CMD="-w"
fi

# Set the LNB Volts
VOLTS_CMD=" "
if [ "$LNBVOLTS" == "h" ]; then
  VOLTS_CMD="-p h"
fi
if [ "$LNBVOLTS" == "v" ]; then
  VOLTS_CMD="-p v"
fi

# Select the correct channel in the stream
PROG=" "
if [ "$CHAN" != "0" ] && [ "$CHAN" != "" ]; then
  PROG="--program "$CHAN
fi

# Adjust timeout for low SRs
if [[ $SYMBOLRATEK -lt 300 ]] && [[ $TSTIMEOUT -ne -1 ]]; then
  let TSTIMEOUT=2*$TSTIMEOUT
  if [[ $SYMBOLRATEK -lt 150 ]]; then
    let TSTIMEOUT=2*$TSTIMEOUT
  fi
fi

TIMEOUT_CMD=" "
if [[ $TSTIMEOUT -ge 500 ]] || [[ $TSTIMEOUT -eq -1 ]]; then
  TIMEOUT_CMD=" -r "$TSTIMEOUT" "
fi

SCAN_CMD=" "
if [[ $SCANWIDTH -ge 10 ]] && [[ $SCANWIDTH -lt 250 ]]; then
  SCAN_CMD=`echo - | awk '{print '$SCANWIDTH' / 100}'`
  SCAN_CMD="-S "$SCAN_CMD
fi

# Error check volume
if [ "$VLCVOLUME" -lt 0 ]; then
  VLCVOLUME=0
fi
if [ "$VLCVOLUME" -gt 512 ]; then
  VLCVOLUME=512
fi

# Create dummy marquee overlay file
echo " " >/home/pi/tmp/vlc_overlay.txt

sudo killall longmynd >/dev/null 2>/dev/null
sudo killall vlc >/dev/null 2>/dev/null

# Play a very short dummy file if this is a first start for VLC since boot
# This makes sure the RX works on first selection after boot
if [[ ! -f /home/pi/tmp/vlcprimed ]]; then
  cvlc -I rc --rc-host 127.0.0.1:1111 -f --codec ffmpeg --no-video-title-show \
    --gain 3 --alsa-audio-device $AUDIO_DEVICE \
    /home/pi/portsdown/videos/blank.ts vlc:quit >/dev/null 2>/dev/null &
  sleep 1
  touch /home/pi/tmp/vlcprimed
  echo shutdown | nc 127.0.0.1 1111
fi

# Create the ts fifo
sudo rm longmynd_main_ts >/dev/null 2>/dev/null
mkfifo longmynd_main_ts

UDPIP=127.0.0.1
#UDPIP=192.168.2.184
UDPPORT=1234

# Start LongMynd

/home/pi/longmynd/longmynd -i $UDPIP $UDPPORT -s longmynd_status_fifo \
  $VOLTS_CMD $TIMEOUT_CMD $SCAN_CMD $INPUT_CMD $FREQ_KHZ $SYMBOLRATEK  &

#  $VOLTS_CMD $TIMEOUT_CMD -A 11 $SCAN_CMD $INPUT_CMD $FREQ_KHZ $SYMBOLRATEK >/dev/null 2>/dev/null &

# If tuned to the QO-100 beacon, start ffplay
if [ "$FFPLAY" == "yes" ]; then

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

  SDL_AUDIODRIVER="alsa" AUDIODEV="$AUDIO_DEVICE" ffplay $VF $VFROTATE udp://127.0.0.1:1234 &
  exit
fi


# Sort display rotation for VLC
if [ "$VLCTRANSFORM" == "0" ]; then
  VLCROTATE=" "
elif [ "$VLCTRANSFORM" == "90" ]; then
  VLCROTATE=":vout-filter=transform --transform-type=90 --video-filter transform{true}"
elif [ "$VLCTRANSFORM" == "180" ]; then
  VLCROTATE=":vout-filter=transform --transform-type=180 --video-filter transform{true}"
elif [ "$VLCTRANSFORM" == "270" ]; then
  VLCROTATE=":vout-filter=transform --transform-type=270 --video-filter transform{true}"
fi


# Start VLC
cvlc -I rc --rc-host 127.0.0.1:1111 $PROG --codec ffmpeg -f --no-video-title-show \
  $VLCROTATE \
  --gain 3 --alsa-audio-device $AUDIO_DEVICE \
  udp://@127.0.0.1:1234 >/dev/null 2>/dev/null &

# /home/pi/portsdown/videos/Big_Buck_Bunny_720_10s_10MB.mp4  &
#  --sub-filter marq --marq-x 25 --marq-file "/home/pi/tmp/vlc_overlay.txt" \
#   --width 800 --height 480 \
#   $OVERLAY \ >/dev/null 2>/dev/null

sleep 1

# Set the start-up volume
printf "volume "$VLCVOLUME"\nlogout\n" | nc 127.0.0.1 1111 >/dev/null 2>/dev/null

# Stop VLC playing and move it to the next track.  This is required for PicoTuner
# as it sends the TS data in 64 byte bursts, not 510 byte bursts like MiniTiouner
# The -i 1 parameter to put a second between commands is required to ensure reset at low SRs
nc -i 1 127.0.0.1 1111 >/dev/null 2>/dev/null << EOF
stop
next
logout
EOF

exit


