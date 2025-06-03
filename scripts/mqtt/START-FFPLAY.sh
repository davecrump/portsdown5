#!/usr/bin/bash
#Script to start up to 3 receivers, by DL5OCD

BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-rx.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/config-rx.ini

# ###################################################

cat ./version.txt

echo "Do you want to decode 1, 2 or 3 streams (press 1, 2 or 3):"
read DUAL


#################### RX1 ############################
if [[ "$DUAL" == 1 ]]; then

cat ./ini/rx.ini

echo "Please, choose your scaling for the video  (0,1,2,3,4,9) :"
read CHOICE

if [[ "$CHOICE" == 0 ]]; then
ffplay udp://@"$IP1":"$PORT1"
exit
fi
if [[ "$CHOICE" == 9 ]]; then
ffplay on1rc@66KS.png
fi
if [[ "$CHOICE" == 1 ]]; then
x=930
y=522
fi
if [[ "$CHOICE" == 2 ]]; then
x=1280
y=720
fi
if [[ "$CHOICE" == 3 ]]; then
x=1980
y=1080
fi
if [[ "$CHOICE" == 4 ]]; then
x=2560
y=1440
fi

ffplay -x $x -y $y udp://@"$IP1":"$PORT1"


fi


#################### RX1+2 ##########################
if [[ "$DUAL" == 2 ]]; then

cat ./ini/rx.ini

echo "Please, choose your scaling for the video  (0,1,2,3,4,9) :"
read CHOICE

if [[ "$CHOICE" == 0 ]]; then
ffplay udp://@"$IP1":"$PORT1"
ffplay udp://@"$IP2":"$PORT2"
exit
fi
if [[ "$CHOICE" == 9 ]]; then
ffplay on1rc@66KS.png
fi
if [[ "$CHOICE" == 1 ]]; then
x=930
y=522
fi
if [[ "$CHOICE" == 2 ]]; then
x=1280
y=720
fi
if [[ "$CHOICE" == 3 ]]; then
x=1980
y=1080
fi
if [[ "$CHOICE" == 4 ]]; then
x=2560
y=1440
fi

ffplay -x $x -y $y udp://@"$IP1":"$PORT1"
ffplay -x $x -y $y udp://@"$IP2":"$PORT2"

fi



#################### RX1+2+3 ########################
if [[ "$DUAL" == 3 ]]; then

cat ./ini/rx.ini

echo "Please, choose your scaling for the video  (0,1,2,3,4,9) :"
read CHOICE

if [[ "$CHOICE" == 0 ]]; then
ffplay udp://@"$IP1":"$PORT1"
ffplay udp://@"$IP2":"$PORT2"
ffplay udp://@"$IP3":"$PORT3"
exit
fi
if [[ "$CHOICE" == 9 ]]; then
ffplay on1rc@66KS.png
fi
if [[ "$CHOICE" == 1 ]]; then
x=930
y=522
fi
if [[ "$CHOICE" == 2 ]]; then
x=1280
y=720
fi
if [[ "$CHOICE" == 3 ]]; then
x=1980
y=1080
fi
if [[ "$CHOICE" == 4 ]]; then
x=2560
y=1440
fi

ffplay -x $x -y $y udp://@"$IP1":"$PORT1"
ffplay -x $x -y $y udp://@"$IP2":"$PORT2"
ffplay -x $x -y $y udp://@"$IP3":"$PORT3"

fi
exit

