#!/usr/bin/bash
#Script to start up one receivers, by DL5OCD

BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-rx.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < ./config-tx.ini

# ###################################################

cat ./version.txt

#################### RX1 ############################


cat ./ini/rx.ini

echo "Please, choose your scaling for the video  (0,1,2,3,4,9) :"
read CHOICE

if [[ "$CHOICE" == 0 ]]; then
ffplay udp://@"$DATVOUTIP":"$DATVOUTPORT"
exit
fi
if [[ "$CHOICE" == 9 ]]; then
ffplay ./on1rc@66KS.png
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

ffplay -x $x -y $y udp://@"$DATVOUTIP":"$DATVOUTPORT"


fi

exit
