#!/bin/bash

# This script is run to compile and enable the extra functions used by the keyLimePi RF analyser

############ Set Environment Variables ###############

SCONFIGFILE=/home/pi/portsdown/configs/system_config.txt

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


############ Function to Read value from Config File ###############

get-config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=[+-]?(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

################## Execution starts here ########################

# Enable KLP in system config
set_config_var keylimepi enabled $SCONFIGFILE

# Enable i2c
sudo raspi-config nonint do_i2c 0

# Compile raw SDR Bandviewer
cd /home/pi/portsdown/src/sa_sdr
touch sa_sdr.c
make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
mv /home/pi/portsdown/src/sa_sdr/sa_sdr /home/pi/portsdown/bin/sa_sdr
cd /home/pi

# Compile sdr on SA IF
cd /home/pi/portsdown/src/sa_bv
touch sa_bv.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
mv /home/pi/portsdown/src/sa_bv/sa_bv /home/pi/portsdown/bin/sa_bv
cd /home/pi

# Compile SA IF
cd /home/pi/portsdown/src/sa_if
touch sa_if.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
mv /home/pi/portsdown/src/sa_if/sa_if /home/pi/portsdown/bin/sa_if
cd /home/pi

# Compile the SA scheduler
cd /home/pi/portsdown/src/sa_sched
touch sa_sched.c
make -j 4 -O 
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
fi
mv /home/pi/portsdown/src/sa_sched/sa_sched /home/pi/portsdown/bin/sa_sched
cd /home/pi




