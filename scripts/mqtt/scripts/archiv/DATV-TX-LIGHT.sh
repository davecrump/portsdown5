#!/usr/bin/bash
# Script for DATV-TX via FFMPEG, by DL5OCD
# DATV-NotSoEasy-light

BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-tx.ini

while read line; do
    export "$line" > /dev/null 2>&1
done < $BASEDIR/config-tx.ini

# ###################################################

cat ./version.txt
echo Running script in $BASEDIR

# You can adjust the lines below when you know what you are doing ;-)
# ###################################################
# Read previous run from params.ini and favorites from favorite-x.ini

echo -------------------------
echo Last run:
cat ./ini/params-light.ini
echo -------------------------
echo -------------------------
echo Profile 1 setup:
cat ./ini/favorite-light-1.ini
echo -------------------------
echo -------------------------
echo Profile 2 setup:
cat ./ini/favorite-light-2.ini
echo -------------------------
echo -------------------------
echo Profile 3 setup:
cat ./ini/favorite-light-3.ini
echo -------------------------


if [ "$FW" == "no" ]; then

echo "FW=yes in config-tx.ini not set, exit"
exit
fi


echo "Use profile 1 (1), profile 2 (2), profile 3 (3), use previous parameters (4), start new parameters (5) :"
read AUTO

if [[ "$AUTO" == 1 ]]; then
while read line; do    
    export $line > /dev/null 2>&1
done < $BASEDIR/ini/favorite-1.ini
fi

if [[ "$AUTO" == 2 ]]; then
while read line; do    
    export $line > /dev/null 2>&1
done < $BASEDIR/ini/favorite-2.ini
fi

if [[ "$AUTO" == 3 ]]; then
while read line; do    
    export $line > /dev/null 2>&1
done < $BASEDIR/ini/favorite-3.ini
fi

if [[ "$AUTO" == 4 ]]; then
while read line; do    
    export $line > /dev/null 2>&1
done < $BASEDIR/ini/params.ini
fi

		if [[ "$AUTO" == 5 ]]; then
	
#MODE
echo "Please, choose your Mode : QPSK (1), 8PSK (2), 16APSK (3) :"
read MODECHOICE

if [[ "$MODECHOICE" == 1 ]]; then
MODE=qpsk
fi
if [[ "$MODECHOICE" == 2 ]]; then
MODE=8psk
fi
if [[ "$MODECHOICE" == 3 ]]; then
MODE=16apsk
fi

# FREQUENCYTX
cat ./ini/frequency.ini
echo "Please, choose your TX-Frequency 0-27 :"
read TXFREQCHOICE

if [[ "$TXFREQCHOICE" == 0 ]]; then
echo "Please, choose your TX-Frequency (70Mhz-6Ghz), input in Hz :"
read TXFREQUENCY

else

if [[ "$TXFREQCHOICE" == 1 ]]; then TXFREQUENCY=2403.25e6
fi
if [[ "$TXFREQCHOICE" == 2 ]]; then TXFREQUENCY=2403.50e6
fi
if [[ "$TXFREQCHOICE" == 3 ]]; then TXFREQUENCY=2403.75e6
fi
if [[ "$TXFREQCHOICE" == 4 ]]; then TXFREQUENCY=2404.00e6
fi
if [[ "$TXFREQCHOICE" == 5 ]]; then TXFREQUENCY=2404.25e6
fi
if [[ "$TXFREQCHOICE" == 6 ]]; then TXFREQUENCY=2404.50e6
fi
if [[ "$TXFREQCHOICE" == 7 ]]; then TXFREQUENCY=2404.75e6
fi
if [[ "$TXFREQCHOICE" == 8 ]]; then TXFREQUENCY=2405.00e6
fi
if [[ "$TXFREQCHOICE" == 9 ]]; then TXFREQUENCY=2405.25e6
fi
if [[ "$TXFREQCHOICE" == 10 ]]; then TXFREQUENCY=2405.50e6
fi
if [[ "$TXFREQCHOICE" == 11 ]]; then TXFREQUENCY=2405.75e6
fi
if [[ "$TXFREQCHOICE" == 12 ]]; then TXFREQUENCY=2406.00e6
fi
if [[ "$TXFREQCHOICE" == 13 ]]; then TXFREQUENCY=2406.25e6
fi
if [[ "$TXFREQCHOICE" == 14 ]]; then TXFREQUENCY=2406.50e6
fi
if [[ "$TXFREQCHOICE" == 15 ]]; then TXFREQUENCY=2406.75e6
fi
if [[ "$TXFREQCHOICE" == 16 ]]; then TXFREQUENCY=2407.00e6
fi
if [[ "$TXFREQCHOICE" == 17 ]]; then TXFREQUENCY=2407.25e6
fi
if [[ "$TXFREQCHOICE" == 18 ]]; then TXFREQUENCY=2407.50e6
fi
if [[ "$TXFREQCHOICE" == 19 ]]; then TXFREQUENCY=2407.75e6
fi
if [[ "$TXFREQCHOICE" == 20 ]]; then TXFREQUENCY=2408.00e6
fi
if [[ "$TXFREQCHOICE" == 21 ]]; then TXFREQUENCY=2408.25e6
fi
if [[ "$TXFREQCHOICE" == 22 ]]; then TXFREQUENCY=2408.50e6
fi
if [[ "$TXFREQCHOICE" == 23 ]]; then TXFREQUENCY=2408.75e6
fi
if [[ "$TXFREQCHOICE" == 24 ]]; then TXFREQUENCY=2409.00e6
fi
if [[ "$TXFREQCHOICE" == 25 ]]; then TXFREQUENCY=2409.25e6
fi
if [[ "$TXFREQCHOICE" == 26 ]]; then TXFREQUENCY=2409.50e6
fi
if [[ "$TXFREQCHOICE" == 27 ]]; then TXFREQUENCY=2409.75e6
fi

fi

echo "Please, choose your TX-Gain (0 to -50dB), 0.5dB steps :"
read GAIN
if (( $(bc <<<"$GAIN > $PWRLIM") )); then
echo "Invalid gain, PWRLIM is set to $PWRLIM dB"
exit
fi




	if [ "$MODECHOICE" == "1" ]; then


#QPSK
cat ./ini/qpsk.ini
echo "Please, choose your TX-SR 0-9 :"
read SRCHOICE

if [[ "$SRCHOICE" == 0 ]]; then
echo "Please, choose your TX-SR (25-4000) :"
read SR

else

if [[ "$SRCHOICE" == 1 ]]; then
SR=35
fi
if [[ "$SRCHOICE" == 2 ]]; then
SR=66
fi
if [[ "$SRCHOICE" == 3 ]]; then
SR=125
fi
if [[ "$SRCHOICE" == 4 ]]; then
SR=250
fi
if [[ "$SRCHOICE" == 5 ]]; then
SR=333
fi
if [[ "$SRCHOICE" == 6 ]]; then
SR=500
fi
if [[ "$SRCHOICE" == 7 ]]; then
SR=1000
fi
if [[ "$SRCHOICE" == 8 ]]; then
SR=1500
fi
if [[ "$SRCHOICE" == 9 ]]; then
SR=2000
fi

fi


cat ./ini/qpsk-fec.ini
echo "Please, choose your TX-FEC 1-11 :"
read FECCHOICE

if [[ "$FECCHOICE" == 1 ]]; then
FEC=1/4
fi
if [[ "$FECCHOICE" == 2 ]]; then
FEC=1/3
fi
if [[ "$FECCHOICE" == 3 ]]; then
FEC=2/5
fi
if [[ "$FECCHOICE" == 4 ]]; then
FEC=1/2
fi
if [[ "$FECCHOICE" == 5 ]]; then
FEC=3/5
fi
if [[ "$FECCHOICE" == 6 ]]; then
FEC=2/3
fi
if [[ "$FECCHOICE" == 7 ]]; then
FEC=3/4
fi
if [[ "$FECCHOICE" == 8 ]]; then
FEC=4/5
fi
if [[ "$FECCHOICE" == 9 ]]; then
FEC=5/6
fi
if [[ "$FECCHOICE" == 10 ]]; then
FEC=8/9
fi
if [[ "$FECCHOICE" == 11 ]]; then
FEC=9/10
fi

	fi




	if [ "$MODECHOICE" == "2" ]; then


#8PSK
cat ./ini/8psk.ini
echo "Please, choose your TX-SR 0-9 :"
read SRCHOICE

if [[ "$SRCHOICE" == 0 ]]; then
echo "Please, choose your TX-SR (25-4000) :"
read SR

else

if [[ "$SRCHOICE" == 1 ]]; then
SR=35
fi
if [[ "$SRCHOICE" == 2 ]]; then
SR=66
fi
if [[ "$SRCHOICE" == 3 ]]; then
SR=125
fi
if [[ "$SRCHOICE" == 4 ]]; then
SR=250
fi
if [[ "$SRCHOICE" == 5 ]]; then
SR=333
fi
if [[ "$SRCHOICE" == 6 ]]; then
SR=500
fi
if [[ "$SRCHOICE" == 7 ]]; then
SR=1000
fi
if [[ "$SRCHOICE" == 8 ]]; then
SR=1500
fi
if [[ "$SRCHOICE" == 9 ]]; then
SR=2000
fi

fi


cat ./ini/8psk-fec.ini
echo "Please, choose your TX-FEC 1-6 :"
read FECCHOICE

if [[ "$FECCHOICE" == 1 ]]; then
FEC=3/5
fi
if [[ "$FECCHOICE" == 2 ]]; then
FEC=2/3
fi
if [[ "$FECCHOICE" == 3 ]]; then
FEC=3/4
fi
if [[ "$FECCHOICE" == 4 ]]; then
FEC=5/6
fi
if [[ "$FECCHOICE" == 5 ]]; then
FEC=8/9
fi
if [[ "$FECCHOICE" == 6 ]]; then
FEC=9/10
fi

	fi




	if [ "$MODECHOICE" == "3" ]; then


#16APSK
cat ./ini/16apsk.ini
echo "Please, choose your TX-SR 0-9 :"
read SRCHOICE

if [[ "$SRCHOICE" == 0 ]]; then
echo "Please, choose your TX-SR (25-4000) :"
read SR

else

if [[ "$SRCHOICE" == 1 ]]; then
SR=35
fi
if [[ "$SRCHOICE" == 2 ]]; then
SR=66
fi
if [[ "$SRCHOICE" == 3 ]]; then
SR=125
fi
if [[ "$SRCHOICE" == 4 ]]; then
SR=250
fi
if [[ "$SRCHOICE" == 5 ]]; then
SR=333
fi
if [[ "$SRCHOICE" == 6 ]]; then
SR=500
fi
if [[ "$SRCHOICE" == 7 ]]; then
SR=1000
fi
if [[ "$SRCHOICE" == 8 ]]; then
SR=1500
fi
if [[ "$SRCHOICE" == 9 ]]; then
SR=2000
fi

fi


cat ./ini/16apsk-fec.ini
echo "Please, choose your TX-FEC 1-6 :"
read FECCHOICE

if [[ "$FECCHOICE" == 1 ]]; then
FEC=2/3
fi
if [[ "$FECCHOICE" == 2 ]]; then
FEC=3/4
fi
if [[ "$FECCHOICE" == 3 ]]; then
FEC=5/6
fi
if [[ "$FECCHOICE" == 4 ]]; then
FEC=8/9
fi
if [[ "$FECCHOICE" == 5 ]]; then
FEC=9/10
fi

	fi

		fi



echo -----------------------------------
echo Running parameters:
echo -----------------------------------
echo Service-Name: $CALLSIGN
echo Service-Provider: $SERVICEPROVIDER
echo Pluto-IP: $PLUTOIP
echo Pluto-Port: $PLUTOPORT
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo TX-SR: "$SR"KS
echo TX-Mode: $MODE
echo TX-FEC: $FEC

# Write run to params.ini
echo SR=$SR > ./ini/params.ini
echo MODE=$MODE >> ./ini/params.ini
echo FEC=$FEC >> ./ini/params.ini
echo TXFREQUENCY=$TXFREQUENCY >> ./ini/params.ini
echo GAIN=$GAIN >> ./ini/params.ini



#MOSQUITTO
# Mosquitto general init commands, don't modify
SRM=$(( $SR * 1000 ))
DIGITALGAIN=0
# Set FEC to min value
if [ "$MODE" = "qpsk" ] && [ "$FECMODE" = "variable" ]; then
FECVARIABLE=1/4
fi
if [ "$MODE" = "8psk" ] && [ "$FECMODE" = "variable" ]; then
FECVARIABLE=3/5
fi
if [ "$MODE" = "16apsk" ] && [ "$FECMODE" = "variable" ]; then
FECVARIABLE=2/3
fi

mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourceaddress -m $PLUTOIP:$PLUTOPORT -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/gain -m $GAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/mute -m $MUTE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/frequency -m $TXFREQUENCY -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/sr -m $SRM -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/nco -m $NCO -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/stream/mode -m $TXMODE -h $PLUTOIP

if [ "$FECMODE" == "fixed" ]; then
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fec -m $FEC -h $PLUTOIP
fi
if [ "$FECMODE" == "variable" ]; then
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fec -m $FECVARIABLE -h $PLUTOIP
fi

mosquitto_pub -t $CMD_ROOT/tx/dvbs2/constel -m $MODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/pilots -m $PILOTS -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/frame -m $FRAME -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecmode -m $FECMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/agcgain -m $AGCGAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/digitalgain -m $DIGITALGAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecrange -m $FECRANGE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcemode -m $TSSOURCEMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcefile -m $TSSOURCEFILE -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/lnb_supply -m $LNBSUPPLY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/polarisation -m $LNBPOL -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/swport -m %TUNERPORT -h $PLUTOIP

# Start Control, decoder and MQTT Browser
xfce4-terminal --title DATV-NotSoEasy-CONTROL -e 'bash ./scripts/CONTROL-LIGHT.sh' &
mqtt-explorer >> /dev/null 2>&1 &


echo "echo Running, now stream the Pluto from OBS or something similar....press any key to exit..."
read

# Kill
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
if [[ "$REBOOT" == "on" ]]; then
mosquitto_pub -t $CMD_ROOT/system/reboot -m 1 -h $PLUTOIP
fi
killall mqtt-explorer
killall xfce4-terminal
# if [[ "$ROUTE" == yes ]]; then
# route delete $ROUTENET
# fi
exit

