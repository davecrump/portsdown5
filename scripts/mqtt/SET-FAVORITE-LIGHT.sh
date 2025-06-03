#!/usr/bin/bash
#Script for setting favorite parameters, by DL5OCD
#DATV-NotSoEasy-Light


BASEDIR=$(pwd)
cd $BASEDIR

# ############## Global configuration ###############
# Read configuration from config-tx.ini

while read line; do    
    export "$line" > /dev/null 2>&1
done < $BASEDIR/config-tx.ini

# ###################################################

echo Script for setting favorite parameters by DL5OCD
echo ------------------------------------------------
echo -
echo ----------------------------
echo Profile 1 parameters:
cat ./ini/favorite-light-1.ini
echo ----------------------------
echo ----------------------------
echo Profile 2 parameters:
cat ./ini/favorite-light-2.ini
echo ----------------------------
echo ----------------------------
echo Profile 3 parameters:
cat ./ini/favorite-light-3.ini
echo ----------------------------

#Run in a loop
while true
do


echo "Which profile do you want to change: Profile 1 (1), profile 2 (2) or profile 3 (3) :"
read PROFILECHOICE

if [[ "$PROFILECHOICE" == 1 ]]; then
PROFILECOUNT=1
fi
if [[ "$PROFILECHOICE" == 2 ]]; then
PROFILECOUNT=2
fi
if [[ "$PROFILECHOICE" == 3 ]]; then
PROFILECOUNT=3
fi


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




# Check parameters
if (( $(bc <<<"$SR > 4000") )); then
echo "Invalid SR >4000KS"
exit
fi

# Write to favorite-light-x.ini
echo -----------------------------------
echo Setting favorite parameters....
echo -----------------------------------

echo SR=$SR > ./ini/favorite-light-"$PROFILECOUNT".ini
echo MODE=$MODE >> ./ini/favorite-light-"$PROFILECOUNT".ini
echo FEC=$FEC >> ./ini/favorite-light-"$PROFILECOUNT".ini
echo TXFREQUENCY=$TXFREQUENCY >> ./ini/favorite-light-"$PROFILECOUNT".ini
echo GAIN=$GAIN >> ./ini/favorite-light-"$PROFILECOUNT".ini

echo --------------------------------------
echo Profile %PROFILECOUNT% parameters are set
echo --------------------------------------

echo "Do you want to change another profile? YES (1), NO (2) :"
read CHANGEMORE

if [[ "$CHANGEMORE" == 2 ]]; then
exit
fi


done

