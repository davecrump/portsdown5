#! /bin/bash

# Compile portsdown gui

# set -x

if pgrep -x "startup.sh" >/dev/null
then
  ps -ef | grep startup.sh | grep -v grep | awk '{print $2}' | xargs kill
fi

if pgrep -f -x "single_screen_grab_for_web.sh" >/dev/null
then
  ps -ef | grep single_screen_grab_for_web.sh | grep -v grep | awk '{print $2}' | xargs kill
fi

sudo killall fb2png >/dev/null 2>/dev/null

sudo killall portsdown5 >/dev/null 2>/dev/null
cd /home/pi/portsdown/src/portsdown
# make clean
touch portsdown5.c

echo Compiling portsdown5.c

make -j 4 -O
if [ $? != "0" ]; then
  echo
  echo "failed install"
  cd /home/pi
  exit
else
  echo
  echo "Successful compile, starting Portsdown 5"
  echo
fi

cd /home/pi

mv /home/pi/portsdown/src/portsdown/portsdown5 /home/pi/portsdown/bin/portsdown5

/home/pi/portsdown/scripts/scheduler.sh --dubug

EXIT_CODE=$?

#reset

echo Program returned exit code $EXIT_CODE



