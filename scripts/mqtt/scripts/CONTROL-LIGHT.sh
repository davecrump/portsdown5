#!/usr/bin/bash
# Script for DATV-TX via FFMPEG, by DL5OCD
# DATV-NotSoEasy

BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-tx.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < ./config-tx.ini

# Read configuration from rxonoff.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < ./ini/rxonoff.ini

# Read configuration from params.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < ./ini/params.ini


DGAIN=0

# Running in a loop
while true
do

echo ---------------------------------------
echo Actual gain is "$GAIN"dB
echo Actual digital gain is "$DGAIN"dB
echo Actual TX-Frequency is "$TXFREQUENCY"Mhz
echo Actual RX-Frequency is "$RXFREQUENCY"Mhz
echo Actual TX-SR is "$SR"KS
echo Actual TX-FEC is $FEC
echo ---------------------------------------

if [[ "$MODE" == "qpsk" ]]; then
echo "Which parameter to change, PTT off (0), PTT on (1), Frequency (2), Gain (3), Digital Gain (4), Mode SR and FEC (5), Advanced Options (6) :"
read CONTROL
fi

if ! [[ "$MODE" == "qpsk" ]]; then
echo "Which parameter to change, PTT off (0), PTT on (1), Frequency (2), Gain (3), Mode SR and FEC (4), Advanced Options (6) :"
read CONTROL
fi


# PTT
if [[ "$CONTROL" == 0 ]]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
echo PTT is OFF
fi
if [[ "$CONTROL" == 1 ]]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 0 -h $PLUTOIP
echo PTT is ON
fi



# FREQUENCYTX
		if [[ "$CONTROL" == 2 ]]; then

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

# TX
mosquitto_pub -t $CMD_ROOT/tx/frequency -m $TXFREQUENCY -h $PLUTOIP
echo TX-Frequency is set to "$TXFREQUENCY"Mhz

	fi
		fi


		if [[ "$CONTROL" == 3 ]]; then
echo "Please, choose your TX-Gain (0 to -50dB), 0.5dB steps :"
read GAIN
if (( $(bc <<<"$GAIN > $PWRLIM") )); then
echo "Invalid gain, PWRLIM is set to $PWRLIM dB"
exit
fi

mosquitto_pub -t $CMD_ROOT/tx/gain -m $GAIN -h $PLUTOIP
echo Gain set to $GAIN


		fi
	
	
	
		if [[ "$CONTROL" == 4 ]]; then
echo "Digital TX-Gain (QPSK only) 3 to -20dB, step is 0.01db :"
read DGAIN
if (( $(bc <<<"$DGAIN > 3") )); then
echo "Invalid gain, value must be between -20 and 3"
if (( $(bc <<<"$DGAIN < 20") )); then
echo "Invalid gain, value must be between -20 and 3"
exit
fi
fi

mosquitto_pub -t $CMD_ROOT/tx/dvbs2/digitalgain -m $DGAIN -h $PLUTOIP
echo Digital TX-Gain set to $DGAIN


		fi




		if [[ "$CONTROL" == 5 ]]; then
		
		
			
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

	if [ "$MODECHOICE" == "1" ]; then
		
#QPSK
cat ./ini/qpsk.ini
echo "Please, choose your TX-SR 0-9 :"
read SRCHOICE

if [[ "$SRCHOICE" == 0 ]]; then
echo "Please, choose your TX-SR (25-3000) :"
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
echo "Please, choose your TX-SR (25-3000) :"
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
echo "Please, choose your TX-SR (25-3000) :"
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
		

# Check parameters
if (( $(bc <<<"$SR > 4000") )); then
echo "Invalid SR >4000KS"
exit
fi

# Status and info
echo -----------------------------------
echo Running parameters:
echo -----------------------------------
echo TX-Frequency: "$TXFREQUENCY"Mhz
echo Gain: "$GAIN"dB
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo -----------------------------------

# Mosquitto commands, don't modify		
SRM=$(( $SR * 1000 ))

mosquitto_pub -t $CMD_ROOT/tx/dvbs2/sr -m $SRM -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fec -m $FEC -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/constel -m $MODE -h $PLUTOIP

		
		fi



		if [[ "$CONTROL" == 6 ]]; then
# Running in a loop until break
while true
do

echo ---------------------------------------
echo Actual FEC is $FEC
echo Actual FECVARIABLE is $FECVARIABLE
echo Actual FECMODE is $FECMODE
echo Actual FECRANGE is $FECRANGE
echo Actual AGCGAIN is "$AGCGAIN"dB
echo Actual GAINVARIABLE is $GAINVARIABLE
echo ---------------------------------------

echo "Which parameter to change, FECMODE (1), FECRANGE (2), AGCGAIN (3), GAINVARIABLE (4), Back to main menu (0) :"
read ADVANCED

	if [[ "$ADVANCED" == 1 ]]; then
echo "Enter FECMODE fixed (1) or variable (2) :"
read FECMODECHOICE
if [[ "$FECMODECHOICE" == 1 ]]; then
FECMODE=fixed
fi
if [[ "$FECMODECHOICE" == 2 ]]; then
FECMODE=variable
fi


if [[ "$FECMODECHOICE" == 2 ]] && [[ "$MODE" == qpsk ]]; then
FECVARIABLE=1/4
fi
if [[ "$FECMODECHOICE" == 2 ]] && [[ "$MODE" == 8psk ]]; then
FECVARIABLE=3/5
fi
if [[ "$FECMODECHOICE" == 2 ]] && [[ "$MODE" == 16apsk ]]; then
FECVARIABLE=2/3
fi

if [[ "$FECMODECHOICE" == 1 ]]; then
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecmode -m $FECMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fec -m $FEC -h $PLUTOIP
echo "FECMODE is set to $FECMODE"
fi
if [[ "$FECMODECHOICE" == 2 ]]; then
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecmode -m $FECMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fec -m $FECVARIABLE -h $PLUTOIP
echo "FECMODE is set to $FECMODE"
fi
	fi


	if [[ "$ADVANCED" == 2 ]]; then
echo "Enter FECRANGE 1-11 :"
read FECRANGE
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecrange -m $FECRANGE -h $PLUTOIP
echo "FECRANGE is set to $FECRANGE"

	fi
	
	
	if [[ "$ADVANCED" == 3 ]]; then
echo "Enter AGCGAIN -1 to -100 (-100 is AGC off) :"
read AGCGAIN
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/agcgain -m $AGCGAIN -h $PLUTOIP
echo "AGCGAIN is set to $AGCGAIN"

	fi
	
	
	if [[ "$ADVANCED" == 4 ]]; then
echo "Enter GAINVARIABLE on (1), off (0) :"
read GAINVARIABLE
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/gainvariable -m $GAINVARIABLE -h $PLUTOIP
echo "GAINVARIABLE is set to $GAINVARIABLE"

	fi


	if [[ "$ADVANCED" == 0 ]]; then
break

	fi
	
	
echo "Change more advanced options? Yes (1) No (0) :"
read CHANGEMORE
if [[ "$CHANGEMORE" == 0 ]]; then
break
fi
done


		fi

done


