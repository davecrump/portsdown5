#!/bin/bash

# set -x

# This script is sourced from .bashrc at boot and ssh session start
# to sort out driver issues and
# to select the user's selected start-up option.
# Dave Crump 20240521

############ Set Environment Variables ###############

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"

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
echo "Entered startup.sh"  > /home/pi/startuplog.txt
echo "Session detected as -"$SESSION_TYPE"-" >> /home/pi/startuplog.txt
whoami >> /home/pi/startuplog.txt
echo "the ssh client is-"$SSH_CLIENT"-" >> /home/pi/startuplog.txt

# If this is the first reboot after install, sort out the sdrplay api and subsequent makes
#if [ -f /home/pi/rpidatv/.post-install_actions ]; then
#  /home/pi/rpidatv/scripts/post-install_actions.sh
#fi

# Read the desired start-up behaviour
MODE_STARTUP=$(get_config_var startup $SCONFIGFILE)

# If pi-sdn is not running, check if it is required to run
#ps -cax | grep 'pi-sdn' >/dev/null 2>/dev/null
#RESULT="$?"
#if [ "$RESULT" -ne 0 ]; then
#  if [ -f /home/pi/.pi-sdn ]; then
#    . /home/pi/.pi-sdn
#  fi
#fi

# Facility to Disable WiFi
# Calls .wifi_off if present and runs "sudo ip link set wlan0 down"
#if [ -f ~/.wifi_off ]; then
#    . ~/.wifi_off
#fi

# If a boot session, put up the BATC Splash Screen, and then kill the process
#sudo fbi -T 1 -noverbose -a /home/pi/rpidatv/scripts/images/BATC_Black.png >/dev/null 2>/dev/null
#(sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work

# Select the appropriate action

case "$MODE_STARTUP" in
  Prompt)
    # Do nothing
    exit
  ;;
  *)
    # Start the Touchscreen Scheduler
    source /home/pi/portsdown/scripts/scheduler.sh
  ;;
esac


