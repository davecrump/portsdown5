#!/bin/bash

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


PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"

MODULATION=$(get_config_var modulation $PCONFIGFILE)
ENCODING=$(get_config_var encoding $PCONFIGFILE)
MODEOUTPUT=$(get_config_var modeoutput $PCONFIGFILE)
FORMAT=$(get_config_var format $PCONFIGFILE)
VIDEOSOURCE=$(get_config_var videosource $PCONFIGFILE)
FREQOUTPUT=$(get_config_var freqoutput $PCONFIGFILE)
SYMBOLRATE=$(get_config_var symbolrate $PCONFIGFILE)
FEC=$(get_config_var fec $PCONFIGFILE)
LIMEGAIN=$(get_config_var limegain $PCONFIGFILE)
VIDEOSOURCE=$(get_config_var videosource $PCONFIGFILE)
AUDIO_PREF=$(get_config_var audio $PCONFIGFILE)

AUDIO_CHANNELS=0


source /home/pi/portsdown/scripts/transmit/tx_functions.sh

detect_audio

detect_video

detect_webcam_type
