#! /bin/bash

# Compile portsdown gui

# set -x

if pgrep -x "startup.sh" >/dev/null
then
  ps -ef | grep startup.sh | grep -v grep | awk '{print $2}' | xargs kill
fi

sudo killall portsdown5 >/dev/null 2>/dev/null

cd /home/pi

/home/pi/portsdown/scripts/scheduler.sh --dubug

EXIT_CODE=$?

#reset

echo Program returned exit code $EXIT_CODE



