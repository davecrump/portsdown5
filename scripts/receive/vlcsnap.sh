#!/bin/bash

# This script takes a vlc snapshot, converts it to a jpg and and saves it to
# file.  It needs the creation of a /home/pi/tmp/ folder
# as a tmpfs by adding an entry in /etc/fstab
# tmpfs           /home/pi/tmp    tmpfs   defaults,noatime,nosuid,size=10m  0  0
# and a /home/pi/snaps folder

# If timeoverlay=on in Portsdown config file, write a second snap with the time overlay in the bottom
# left corner

#set -x

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

###########################################################################

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"

# Make sure that there aren't any previous temp snaps
sudo rm /home/pi/tmp/vlcsnap.* >/dev/null 2>/dev/null

# If the index number does not exist, create it as zero
if  [ ! -f "/home/pi/snaps/snap_index.txt" ]; then
  echo '0' > /home/pi/snaps/snap_index.txt
fi

# Read the index number
SNAP_SERIAL=$(head -c 4 /home/pi/snaps/snap_index.txt)

# Make sure that there are no previous snaps
rm /home/pi/vlcsnap*.png >/dev/null 2>/dev/null

# Take the snap
printf "snapshot\nlogout\n" | nc 127.0.0.1 1111 >/dev/null 2>/dev/null

# move it to the temp folder
mv vlcsnap*.png /home/pi/tmp/vlcsnap.png

# Convert it to a jpg
convert /home/pi/tmp/vlcsnap.png /home/pi/tmp/vlcsnap.jpg

# Move it to the snap folder 
mv /home/pi/tmp/vlcsnap.jpg /home/pi/snaps/snap"$SNAP_SERIAL".jpg

# Increment the stored index number
let SNAP_SERIAL=$SNAP_SERIAL+1
rm  /home/pi/snaps/snap_index.txt
echo $SNAP_SERIAL  >  /home/pi/snaps/snap_index.txt

# Blink the display to indicate a successful snap
#/home/pi/portsdown/scripts/receive/blink.sh &  # Not required as VLC indicates snap now

# Now write a second snap with a timestamp if required

TIMEOVERLAY=$(get_config_var timeoverlay $PCONFIGFILE)

if [ "$TIMEOVERLAY" == "on" ]; then
  CNGEOMETRY="800x480"
  # Look up time
  SNAPTIME=$(date -u '+%H:%M')
  SNAPDATE=$(date -u '+%d %b %Y')

  convert /home/pi/tmp/vlcsnap.jpg \
    -font "FreeSans" -size "${CNGEOMETRY}" \
    -gravity SouthWest -pointsize 15 -fill grey90 -annotate 0,0,15,15 "$SNAPTIME"$'\n'"$SNAPDATE" \
    /home/pi/tmp/vlcsnap.jpg

  mv /home/pi/tmp/vlcsnap.jpg /home/pi/snaps/snap"$SNAP_SERIAL".jpg

  # Increment the stored index number
  let SNAP_SERIAL=$SNAP_SERIAL+1
  rm  /home/pi/snaps/snap_index.txt
  echo $SNAP_SERIAL  >  /home/pi/snaps/snap_index.txt
fi

# Delete the temporary files
rm /home/pi/tmp/vlcsnap.* >/dev/null 2>/dev/null

exit

