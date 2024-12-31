#!/bin/bash

# set -x

# This script is sourced from startup.sh and run on boot before the Portsdown scheduler
# It checks the requested display parameters and the connected display
# and sorts the cmdline.txt file and system_config.txt, rebooting if required
# Dave Crump 20241218

############ Set Environment Variables ###############

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"
REBOOT_REQUIRED="false"

############ Function to Read from Config File ###############

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

############ Function to Write to Config File ###############

set_config_var() {
lua - "$1" "$2" "$3" <<EOF > "$3.bak"
local key=assert(arg[1])
local value=assert(arg[2])
local fn=assert(arg[3])
local file=assert(io.open(fn))
local made_change=false
for line in file:lines() do
if line:match("^#?%s*"..key.."=.*$") then
line=key.."="..value
made_change=true
end
print(line)
end
if not made_change then
print(key.."="..value)
end
EOF
mv "$3.bak" "$3"
}

######################### Start here #####################

# First determine what display is connected:

kmsprint -l | grep -q 'DSI-1 (connected)'
if [ "$?" == "0" ]; then   ## touchscreen connected
  echo "DETECTED_DISPLAY=touchscreen1"
  DETECTED_DISPLAY="touchscreen1"
else
  kmsprint -l | grep -q 'HDMI-A-1 (connected)'
  if [ "$?" == "0" ]; then   ## hdmi connected
    echo "DETECTED_DISPLAY=hdmi"
    DETECTED_DISPLAY="hdmi"
  else
    kmsprint -l | grep -q 'DSI-2 (connected)'
    if [ "$?" == "0" ]; then   ## touchscreen 2 connected
      echo "DETECTED_DISPLAY=touchscreen2"
      DETECTED_DISPLAY="touchscreen2"
    else
      echo "DETECTED_DISPLAY=none"
      DETECTED_DISPLAY="none"
    fi
  fi
fi

# Check Raspberry Pi Version

cat /sys/firmware/devicetree/base/model | grep -q 'Pi 5 Model'
if [ "$?" == "0" ]; then   ## Pi 5
  PI_MODEL="5"
fi
cat /sys/firmware/devicetree/base/model | grep -q 'Pi 500 Rev'
if [ "$?" == "0" ]; then   ## Pi 500
  PI_MODEL="500"
fi
cat /sys/firmware/devicetree/base/model | grep -q 'Pi 4 Model'
if [ "$?" == "0" ]; then   ## Pi 4
  PI_MODEL="4"
fi
echo PI-MODEL="$PI_MODEL"

# Check if Touchscreen 1 is mounted inverted

TOUCHSCREEN_ORIENTATION=$(get_config_var touch $SCONFIGFILE)
echo "TOUCHSCREEN_ORIENTATION="$TOUCHSCREEN_ORIENTATION

# Check requested HDMI Resolution

HDMI_RESOLUTION=$(get_config_var hdmiresolution $SCONFIGFILE)
echo "HDMI_RESOLUTION="$HDMI_RESOLUTION

# Check requested HDMI resolution against cmdline.txt.  Amend if required.

if [ "$HDMI_RESOLUTION" == "720" ]; then
  cat /boot/firmware/cmdline.txt | grep -q 'video=HDMI-A-1:1280x720M@60'
  if [ "$?" != "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=HDMI-A-1:1920x1080M@60/video=HDMI-A-1:1280x720M@60/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

if [ "$HDMI_RESOLUTION" == "1080" ]; then
  cat /boot/firmware/cmdline.txt | grep -q 'video=HDMI-A-1:1920x1080M@60'
  if [ "$?" != "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=HDMI-A-1:1280x720M@60/video=HDMI-A-1:1920x1080M@60/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

# Read settings in config file

CONFIG_DISPLAY=$(get_config_var display $SCONFIGFILE)
CONFIG_VLC_TRANSFORM=$(get_config_var vlctransform $SCONFIGFILE)
CONFIG_FB_ORIENTATION=$(get_config_var fborientation $SCONFIGFILE)

# Check each use-case in turn

# 1. RPi 5 inverted touchscreen 1 (web control available)

if [ "$PI_MODEL" == "5" ] && [ "$TOUCHSCREEN_ORIENTATION" == "inverted" ] && [ "$DETECTED_DISPLAY" == "touchscreen1" ]; then

  echo "RPi 5 inverted touchscreen 1"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "Element14_7" ]; then  # needs changing
    set_config_var display "Element14_7" $SCONFIGFILE
    CONFIG_DISPLAY="Element14_7"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "180" ]; then  # needs changing
    set_config_var vlctransform "180" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="180"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "180" ]; then  # needs changing
    set_config_var fborientation "180" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="180"
  fi

  # Check touchscreen entry in cmdline.txt
  cat /boot/firmware/cmdline.txt | grep -q 'video=DSI-1:800x480M-16@60,rotate=180'
  if [ "$?" != "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=DSI-1:800x480M-16@60/video=DSI-1:800x480M-16@60,rotate=180/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

# 2. RPi 5 non-inverted touchscreen 1 (web control available)

if [ "$PI_MODEL" == "5" ] && [ "$TOUCHSCREEN_ORIENTATION" == "normal" ] && [ "$DETECTED_DISPLAY" == "touchscreen1" ]; then

  echo "RPi 5 non-inverted touchscreen 1"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "Element14_7" ]; then  # needs changing
    set_config_var display "Element14_7" $SCONFIGFILE
    CONFIG_DISPLAY="Element14_7"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "0" ]; then  # needs changing
    set_config_var vlctransform "0" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="0"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "0" ]; then  # needs changing
    set_config_var fborientation "0" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="0"
  fi

  # Check touchscreen entry in cmdline.txt
  cat /boot/firmware/cmdline.txt | grep -q 'video=DSI-1:800x480M-16@60,rotate=180'
  if [ "$?" == "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=DSI-1:800x480M-16@60,rotate=180/video=DSI-1:800x480M-16@60/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

# 3. RPi 4 inverted touchscreen 1 (web control available)

if [ "$PI_MODEL" == "4" ] && [ "$TOUCHSCREEN_ORIENTATION" == "inverted" ] && [ "$DETECTED_DISPLAY" == "touchscreen1" ]; then

  echo "RPi 4 inverted touchscreen 1"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "Element14_7" ]; then  # needs changing
    set_config_var display "Element14_7" $SCONFIGFILE
    CONFIG_DISPLAY="Element14_7"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "0" ]; then  # needs changing
    set_config_var vlctransform "0" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="0"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "0" ]; then  # needs changing
    set_config_var fborientation "0" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="0"
  fi

  # Check touchscreen entry in cmdline.txt
  cat /boot/firmware/cmdline.txt | grep -q 'video=DSI-1:800x480M-16@60,rotate=180'
  if [ "$?" != "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=DSI-1:800x480M-16@60/video=DSI-1:800x480M-16@60,rotate=180/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

# 4. RPi 4 non-inverted touchscreen 1 (web control available)

if [ "$PI_MODEL" == "4" ] && [ "$TOUCHSCREEN_ORIENTATION" == "normal" ] && [ "$DETECTED_DISPLAY" == "touchscreen1" ]; then

  echo "RPi 4 non-inverted touchscreen 1"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "Element14_7" ]; then  # needs changing
    set_config_var display "Element14_7" $SCONFIGFILE
    CONFIG_DISPLAY="Element14_7"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "0" ]; then  # needs changing
    set_config_var vlctransform "0" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="0"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "0" ]; then  # needs changing
    set_config_var fborientation "0" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="0"
  fi

  # Check touchscreen entry in cmdline.txt
  cat /boot/firmware/cmdline.txt | grep -q 'video=DSI-1:800x480M-16@60,rotate=180'
  if [ "$?" == "0" ]; then  # needs correcting
    sudo sed -i -e 's/video=DSI-1:800x480M-16@60,rotate=180/video=DSI-1:800x480M-16@60/g' /boot/firmware/cmdline.txt
    REBOOT_REQUIRED="true"
  fi
fi

# 5. RPi 4 or 5 with no display (web control)

if [ "$DETECTED_DISPLAY" == "none" ]; then

echo "RPi 4 or 5 with no display"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "web" ]; then  # needs changing
    set_config_var display "web" $SCONFIGFILE
    CONFIG_DISPLAY="Element14_7"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "0" ]; then  # needs changing
    set_config_var vlctransform "0" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="0"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "0" ]; then  # needs changing
    set_config_var fborientation "0" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="0"
  fi
fi

# 6. RPi 4 or 5 with HDMI and mouse (web control available)

if [ "$DETECTED_DISPLAY" == "hdmi" ]; then

echo "RPi 4 or 5 with HDMI and mouse"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "hdmi" ]; then  # needs changing
    set_config_var display "hdmi" $SCONFIGFILE
    CONFIG_DISPLAY="hdmi"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "0" ]; then  # needs changing
    set_config_var vlctransform "0" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="0"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "0" ]; then  # needs changing
    set_config_var fborientation "0" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="0"
  fi
fi

# 7. RPi 5 with touchscreen 2 (web control available)

if [ "$PI_MODEL" == "5" ] &&  [ "$DETECTED_DISPLAY" == "touchscreen2" ]; then

echo "RPi 5 with touchscreen 2"

  # Check display in config file.
  if [ "$CONFIG_DISPLAY" != "touch2" ]; then  # needs changing
    set_config_var display "touch2" $SCONFIGFILE
    CONFIG_DISPLAY="touch2"
  fi

  # Check VLC Transform in config file.
  if [ "$CONFIG_VLC_TRANSFORM" != "90" ]; then  # needs changing
    set_config_var vlctransform "90" $SCONFIGFILE
    CONFIG_VLC_TRANSFORM="90"
  fi

  # Check Frame Buffer invert in config file.
  if [ "$CONFIG_FB_ORIENTATION" != "90" ]; then  # needs changing
    set_config_var fborientation "90" $SCONFIGFILE
    CONFIG_FB_ORIENTATION="90"
  fi
fi

if [ "$REBOOT_REQUIRED" == "true" ]; then
  echo "Rebooting to apply changes"
  sudo reboot now
else
  echo "No display changes needing reboot required"
fi


# The framebuffer needs to built according to the FBorientation.  Then the .bmp for 
# the web and screen capture needs to be built to take pixels in the wrong order to turn it the right way up.

# Note that connection of an HDMI screen needs to direct all output that way, and none to the touchscreen.
# So VLC needs explicitly telling what to do.
# hdmi must always be on hdmi0.  old touchscreen must always be on cam/disp0, nearest the network socket
# New touchscreen must always be on cam/disp 1, furthest from the network.  
# New touchscreen must always be landscape with PCB print the correct way up.
