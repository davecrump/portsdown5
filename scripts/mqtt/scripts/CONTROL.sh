#!/usr/bin/bash
# Script for DATV-TX via FFMPEG, by DL5OCD
# DATV-NotSoEasy

BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-tx.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/config-tx.ini

# Read configuration from rxonoff.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/ini/rxonoff.ini

# Read configuration from relay.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/ini/relay.ini

# Read configuration from params.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/ini/params.ini


DGAIN=0

# Running in a loop
while true
do

echo ---------------------------------------
echo Actual gain is "$GAIN"dB
echo Actual digital gain is "$DGAIN"dB
echo Actual TX-Frequency is "$TXFREQUENCY"Mhz
if [[ "$RXL" == "on" ]]; then
echo Actual RX-Frequency is "$RXFREQUENCY"Mhz
echo Actual RX-SR is "RXSR"KS
fi
echo ---------------------------------------


if [[ "$MODE" == "qpsk" ]]; then
echo "Which parameter to change, PTT off (0), PTT on (1), Frequency (2), Gain (3), Digital Gain (4), Advanced Options (5) :"
read CONTROL
fi

if ! [[ "$MODE" == "qpsk" ]]; then
echo "Which parameter to change, PTT off (0), PTT on (1), Frequency (2), Gain (3), Advanced Options (5) :"
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

	fi


	if [[ "$RXL" == "on" ]]; then

cat ./ini/frequency.ini
echo "Please, choose your RX-Frequency 0-28 :"
read RXFREQCHOICE

if [[ "$RXFREQCHOICE" == 0 ]]; then
echo "Please, choose your RX-Frequency (70Mhz-6Ghz), input in kHz :"
read RXFREQUENCY

else

if [[ "$RXFREQCHOICE" == 1 ]]; then RXFREQUENCY=10492750
fi
if [[ "$RXFREQCHOICE" == 2 ]]; then RXFREQUENCY=10493000
fi
if [[ "$RXFREQCHOICE" == 3 ]]; then RXFREQUENCY=10493250
fi
if [[ "$RXFREQCHOICE" == 4 ]]; then RXFREQUENCY=10493500
fi
if [[ "$RXFREQCHOICE" == 5 ]]; then RXFREQUENCY=10493750
fi
if [[ "$RXFREQCHOICE" == 6 ]]; then RXFREQUENCY=10494000
fi
if [[ "$RXFREQCHOICE" == 7 ]]; then RXFREQUENCY=10494250
fi
if [[ "$RXFREQCHOICE" == 8 ]]; then RXFREQUENCY=10494500
fi
if [[ "$RXFREQCHOICE" == 9 ]]; then RXFREQUENCY=10494750
fi
if [[ "$RXFREQCHOICE" == 10 ]]; then RXFREQUENCY=10495000
fi
if [[ "$RXFREQCHOICE" == 11 ]]; then RXFREQUENCY=10495250
fi
if [[ "$RXFREQCHOICE" == 12 ]]; then RXFREQUENCY=10495500
fi
if [[ "$RXFREQCHOICE" == 13 ]]; then RXFREQUENCY=10495750
fi
if [[ "$RXFREQCHOICE" == 14 ]]; then RXFREQUENCY=10496000
fi
if [[ "$RXFREQCHOICE" == 15 ]]; then RXFREQUENCY=10496250
fi
if [[ "$RXFREQCHOICE" == 16 ]]; then RXFREQUENCY=10496500
fi
if [[ "$RXFREQCHOICE" == 17 ]]; then RXFREQUENCY=10496750
fi
if [[ "$RXFREQCHOICE" == 18 ]]; then RXFREQUENCY=10497000
fi
if [[ "$RXFREQCHOICE" == 19 ]]; then RXFREQUENCY=10497250
fi
if [[ "$RXFREQCHOICE" == 20 ]]; then RXFREQUENCY=10497500
fi
if [[ "$RXFREQCHOICE" == 21 ]]; then RXFREQUENCY=10497750
fi
if [[ "$RXFREQCHOICE" == 22 ]]; then RXFREQUENCY=10498000
fi
if [[ "$RXFREQCHOICE" == 23 ]]; then RXFREQUENCY=10498250
fi
if [[ "$RXFREQCHOICE" == 24 ]]; then RXFREQUENCY=10498500
fi
if [[ "$RXFREQCHOICE" == 25 ]]; then RXFREQUENCY=10498750
fi
if [[ "$RXFREQCHOICE" == 26 ]]; then RXFREQUENCY=10499000
fi
if [[ "$RXFREQCHOICE" == 27 ]]; then RXFREQUENCY=10499250
fi
if [[ "$RXFREQCHOICE" == 28 ]]; then RXFREQUENCY=10491500
fi

fi

	fi
	
# TX
mosquitto_pub -t $CMD_ROOT/tx/frequency -m $TXFREQUENCY -h $PLUTOIP
echo TX-Frequency is set to "$TXFREQUENCY"Mhz

#GSE RX
if [[ "$RXL" == "on" ]]; then
LONGFREQUENCY=$(( $RXFREQUENCY - $RXOFFSET * 1000 ))
mosquitto_pub -t cmd/longmynd/frequency -m $LONGFREQUENCY -h $PLUTOIP
fi

# RXSR
	if [[ "$RXL" == "on" ]]; then

cat ./ini/longmynd-sr.ini
echo "Please, choose your RX-Sample Rate (0-9) :"
read SRCHOICE

if [[ "$SRCHOICE" == 0 ]]; then
echo "Please, choose your RX-SR (25-4000) :"
read RXSR

else

if [[ "$SRCHOICE" == 1 ]]; then
RXSR=35
fi
if [[ "$SRCHOICE" == 2 ]]; then
RXSR=66
fi
if [[ "$SRCHOICE" == 3 ]]; then
RXSR=125
fi
if [[ "$SRCHOICE" == 4 ]]; then
RXSR=250
fi
if [[ "$SRCHOICE" == 5 ]]; then
RXSR=333
fi
if [[ "$SRCHOICE" == 6 ]]; then
RXSR=500
fi
if [[ "$SRCHOICE" == 7 ]]; then
RXSR=1000
fi
if [[ "$SRCHOICE" == 8 ]]; then
RXSR=1500
fi
if [[ "$SRCHOICE" == 9 ]]; then
RXSR=2000
fi

fi
	fi

if [[ "$RXL" == "on" ]]; then
mosquitto_pub -t cmd/longmynd/sr -m $RXSR -h $PLUTOIP

echo Longmynd-Frequency is set to "$LONGFREQUENCY"Khz
echo RX-Frequency is set to "$RXFREQUENCY"Mhz
echo RX-Sample Rate is set to "$RXSR"KS

fi



if [ "$DATVOUT" = "on" ] && [ "$RELAY" = "off" ]; then
killall ffplay
xfce4-terminal -e 'bash ./scripts/START-FFPLAY-LONGMYND.sh' &
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


