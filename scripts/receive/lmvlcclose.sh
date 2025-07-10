#!/bin/bash

# Called (as a forked thread) to stop VLC nicely
# Only works for a UDP stream otherwise longmynd's output fifo gets blocked

SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"

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

FBORIENTATION=$(get_config_var fborientation $SCONFIGFILE)

# Display logo to indicate receive has stopped
case "$FBORIENTATION" in
  0)
    sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black.png >/dev/null 2>/dev/null
  ;;
  90)
    sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black90.png >/dev/null 2>/dev/null
  ;;
  180)
    sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black180.png >/dev/null 2>/dev/null
  ;;
  270)
    sudo fbi -T 1 -noverbose -a /home/pi/portsdown/images/RX_Black270.png >/dev/null 2>/dev/null
  ;;
esac

(sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work

sudo killall ffplay >/dev/null 2>/dev/null

# The -i 1 parameter to put a second between commands is required to ensure reset at low SRs

nc -i 1 127.0.0.1 1111 >/dev/null 2>/dev/null << EOF
shutdown
quit
EOF

VLC_RUNNING=0

while [ $VLC_RUNNING -eq 0 ]
do
  sleep 0.5
  ps -el | grep usr/bin/vlc
  RESPONSE=$?

  if [ $RESPONSE -ne 0 ]; then  #  vlc has stopped
    VLC_RUNNING=1
  else                   # need to shutdown vlc
    sudo killall vlc
  fi
done

exit





