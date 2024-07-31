#! /bin/bash

# Stop all running apps

# set -x

if pgrep -x "startup.sh" >/dev/null
then
  ps -ef | grep startup.sh | grep -v grep | awk '{print $2}' | xargs kill
fi

sudo killall portsdown5 > /dev/null 2>/dev/null
sudo killall limeview > /dev/null 2>/dev/null
sudo killall limeviewng > /dev/null 2>/dev/null

sudo killall netcat > /dev/null 2>/dev/null
sudo killall limedvb > /dev/null 2>/dev/null



