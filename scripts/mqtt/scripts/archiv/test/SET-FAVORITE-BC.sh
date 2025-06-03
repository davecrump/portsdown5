#!/usr/bin/bash
#Script for setting favorite parameters, by DL5OCD
#DATV-NotSoEasy


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
cat ./ini/favorite-1.ini
echo ----------------------------
echo ----------------------------
echo Profile 2 parameters:
cat ./ini/favorite-2.ini
echo ----------------------------
echo ----------------------------
echo Profile 3 parameters:
cat ./ini/favorite-3.ini
echo ----------------------------

#Run in a loop
while true
do

echo "Do you want to edit a normal or GSE profile? Normal (1), GSE (2) :"
read CHANGE


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


# Helper
if [ "$CHANGE" == "1" ]; then
AUTO=5
fi
if [ "$CHANGE" == "2" ]; then
AUTO=6
fi




	if [[ "$AUTO" == 5 ]]; then



if [ "$ENCTYPE" == "soft" ]; then


#CODECSOFT
# echo "Please, choose your Codec: H264 (1), H265 (2), VVC (H266) (3), AV1 (4) :"
echo "Please, choose your Codec (SW-ENC): H264 (1), H265 (2), VVC (H266) (3) :"
read CODECCHOICE

if [[ "$CODECCHOICE" == 1 ]]; then
CODEC=libx264
fi
if [[ "$CODECCHOICE" == 2 ]]; then
CODEC=libx265
fi
if [[ "$CODECCHOICE" == 3 ]]; then
CODEC=libvvenc
fi
if [[ "$CODECCHOICE" == 4 ]]; then
CODEC=libaom-av1
fi

fi

if [ "$ENCTYPE" == "nvidia" ]; then

#CODECHARD
# echo "Please, choose your Codec: H264 (1), H265 (2), VVC (H266) (3), AV1 (4) :"
echo "Please, choose your Codec (HW-ENC): H264 (1), H265 (2), VVC (H266) (3) :"
read CODECCHOICE

if [[ "$CODECCHOICE" == 1 ]]; then
CODEC=h264_nvenc
fi
if [[ "$CODECCHOICE" == 2 ]]; then
CODEC=hevc_nvenc
fi
if [[ "$CODECCHOICE" == 3 ]]; then
CODEC=libvvenc
fi
if [[ "$CODECCHOICE" == 4 ]]; then
CODEC=libaom-av1
fi

fi



#FPS
echo "Please, choose your FPS: 10 (1), 20 (2), 25 (3), 30 (4), 48 (5), 50 (6), 60 (7) :"
read FPSCHOICE

if [[ "$FPSCHOICE" == 1 ]]; then
FPS=10
fi
if [[ "$FPSCHOICE" == 2 ]]; then
FPS=20
fi
if [[ "$FPSCHOICE" == 3 ]]; then
FPS=25
fi
if [[ "$FPSCHOICE" == 4 ]]; then
FPS=30
fi
if [[ "$FPSCHOICE" == 5 ]]; then
FPS=48
fi
if [[ "$FPSCHOICE" == 6 ]]; then
FPS=50
fi
if [[ "$FPSCHOICE" == 7 ]]; then
FPS=60
fi



#5.1
if [ "$AUDIOCODEC" == "ac3" ]; then
AUDIO=6

else

echo "Please, choose Mono (1), or Stereo (2) :"
read AUDIOCHOICE

if [[ "$AUDIOCHOICE" == 1 ]]; then
AUDIO=1
fi
if [[ "$AUDIOCHOICE" == 2 ]]; then
AUDIO=2
fi

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

#Scaling
echo "Please, choose your Image size : 640x360 (1), 960x540 (2), 1280x720 (3), 1920x1080 (4), 2560x1440 (5) :"
read IMAGECHOICE

if [[ "$IMAGECHOICE" == 1 ]]; then
IMAGESIZE=640x360
fi
if [[ "$IMAGECHOICE" == 2 ]]; then
IMAGESIZE=960x540
fi
if [[ "$IMAGECHOICE" == 3 ]]; then
IMAGESIZE=1280x720
fi
if [[ "$IMAGECHOICE" == 4 ]]; then
IMAGESIZE=1920x1080
fi
if [[ "$IMAGECHOICE" == 5 ]]; then
IMAGESIZE=2560x1440
fi

#Audio
echo "Please, choose your Audiobitrate 8k (1), 16k (2), 32k (3), 48k (4), 64 (5), 96k (6) :"
read ABITCHOICE

if [[ "$ABITCHOICE" == 1 ]]; then
ABITRATE=8
fi
if [[ "$ABITCHOICE" == 2 ]]; then
ABITRATE=16
fi
if [[ "$ABITCHOICE" == 3 ]]; then
ABITRATE=32
fi
if [[ "$ABITCHOICE" == 4 ]]; then
ABITRATE=48
fi
if [[ "$ABITCHOICE" == 5 ]]; then
ABITRATE=64
fi
if [[ "$ABITCHOICE" == 6 ]]; then
ABITRATE=96
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


# Calculation QPSK
if [ "$DVBMODE" == "DVB-S2" ]; then
#TSBITRATE=$(( 2 * $SR * 188 / 204 * $FEC * 1075/1000 ))
TSBITRATE=$(echo 2*$SR*188/204*$FEC*1076/1000 | bc -l | cut -c 1-6)
fi

if [ "$DVBMODE" == "DVB-S" ]; then
#TSBITRATE=$(( 2 * $SR * 188 / 204 * $FEC ))
TSBITRATE=$(echo 2*$SR*188/204*$FEC | bc -l | cut -c 1-6)
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


# Calculation 8PSK
if [ "$DVBMODE" == "DVB-S2" ]; then
#TSBITRATE=$(( 3 * $SR * 188 / 204 * $FEC * 1075/1000 ))
TSBITRATE=$(echo 3*$SR*188/204*$FEC*1076/1000 | bc -l | cut -c 1-6)
fi

if [ "$DVBMODE" == "DVB-S" ]; then
#TSBITRATE=$(( 3 * $SR * 188 / 204 * $FEC ))
TSBITRATE=$(echo 3*$SR*188/204*$FEC | bc -l | cut -c 1-6)
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


# Calculation 16APSK
if [ "$DVBMODE" == "DVB-S2" ]; then
#TSBITRATE=$(( 4 * $SR * 188 / 204 * $FEC * 1075/1000 ))
TSBITRATE=$(echo 4*$SR*188/204*$FEC*1076/1000 | bc -l | cut -c 1-6)
fi

if [ "$DVBMODE" == "DVB-S" ]; then
#TSBITRATE=$(( 4 * $SR * 188 / 204 * $FEC ))
TSBITRATE=$(echo 4*$SR*188/204*$FEC | bc -l | cut -c 1-6)
fi

fi

 Videobitrate calculation
# Up to 35K
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 50 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*46/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 66K
if (( $(bc <<<"$SR > 35 && $SR < 67") )); then 
#VBITRATE=$(( 1000 * $TSBITRATE * 60 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*60/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 125K
if (( $(bc <<<"$SR > 66 && $SR < 126") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 68 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*68/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 250K
if (( $(bc <<<"$SR > 125 && $SR < 251") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 76 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*76/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 333K
if (( $(bc <<<"$SR > 250 && $SR < 334") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 80 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*80/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 500K
if (( $(bc <<<"$SR > 333 && $SR < 501") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 82 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*82/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 1000K
if (( $(bc <<<"$SR > 500 && $SR < 1001") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 86 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*86/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 1500K
if (( $(bc <<<"$SR > 1000 && $SR < 1501") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 87 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*87/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Up to 3000K
if (( $(bc <<<"$SR > 1500 && $SR < 3001") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 87 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*87/100-$ABITRATE | bc -l | cut -c 1-6)
fi
# Above 4000K leads to invalid parameter check
if (( $(bc <<<"$SR > 3000") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 88 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*88/100-$ABITRATE | bc -l | cut -c 1-6)
fi



# Decision for adjustment
if [ "$MODE" == "qpsk" ]; then

#ADJUSTQPSK
# Adjusting for low/high FEC

if [ "$FEC" == "1/4" ]; then
#VBITRATE=$(( $VBITRATE * 92 / 100 ))
VBITRATE=$(echo $VBITRATE*92/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "1/3" ]; then
#VBITRATE=$(( $VBITRATE * 94 / 100 ))
VBITRATE=$(echo $VBITRATE*94/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "2/5" ]; then
#VBITRATE=$(( $VBITRATE * 96 / 100 ))
VBITRATE=$(echo $VBITRATE*96/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "1/2" ]; then
#VBITRATE=$(( $VBITRATE * 98 / 100 ))
VBITRATE=$(echo $VBITRATE*98/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "3/5" ]; then
#VBITRATE=$(( $VBITRATE * 99 / 100 ))
VBITRATE=$(echo $VBITRATE*99/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "2/3" ]; then
#VBITRATE=$(( $VBITRATE * 100 / 100 ))
VBITRATE=$(echo $VBITRATE*100/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "3/4" ]; then
#VBITRATE=$(( $VBITRATE * 101 / 100 ))
VBITRATE=$(echo $VBITRATE*101/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "4/5" ]; then
#VBITRATE=$(( $VBITRATE * 101 / 100 ))
VBITRATE=$(echo $VBITRATE*101/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "5/6" ]; then
#VBITRATE=$(( $VBITRATE * 102 / 100 ))
VBITRATE=$(echo $VBITRATE*102/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "8/9" ]; then
#VBITRATE=$(( $VBITRATE * 104 / 100 ))
VBITRATE=$(echo $VBITRATE*104/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "9/10" ]; then
#VBITRATE=$(( $VBITRATE * 105 / 100 ))
VBITRATE=$(echo $VBITRATE*105/100 | bc -l | cut -c 1-6)
fi

if (( $(bc <<<"$VBITRATE > 150 && $VBITRATE < 200") )); then
VBITRATE=$(( $VBITRATE * 92 / 100 ))
fi
if (( $(bc <<<"$VBITRATE > 80 && $VBITRATE < 151") )); then
VBITRATE=$(( $VBITRATE * 90 / 100 ))
fi
if (( $(bc <<<"$VBITRATE > 20 && $VBITRATE < 81") )); then
VBITRATE=$(( $VBITRATE * 68 / 100 ))
fi

fi


# Decision for adjustment
if [ "$MODE" == "8psk" ]; then

#ADJUST8PSK
# Adjusting for low/high FEC

if [ "$FEC" == "3/5" ]; then
#VBITRATE=$(( $VBITRATE * 102 / 100 ))
VBITRATE=$(echo $VBITRATE*102/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "2/3" ]; then
#VBITRATE=$(( $VBITRATE * 104 / 100 ))
VBITRATE=$(echo $VBITRATE*104/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "3/4" ]; then
#VBITRATE=$(( $VBITRATE * 104 / 100 ))
VBITRATE=$(echo $VBITRATE*104/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "5/6" ]; then
#VBITRATE=$(( $VBITRATE * 105 / 100 ))
VBITRATE=$(echo $VBITRATE*105/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "8/9" ]; then
#VBITRATE=$(( $VBITRATE * 106 / 100 ))
VBITRATE=$(echo $VBITRATE*106/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "9/10" ]; then
#VBITRATE=$(( $VBITRATE * 106 / 100 ))
VBITRATE=$(echo $VBITRATE*106/100 | bc -l | cut -c 1-6)
fi

fi


# Decision for adjustment
if [ "$MODE" == "16apsk" ]; then

#ADJUST16APSK
# Adjusting for low/high FEC

if [ "$FEC" == "2/3" ]; then
#VBITRATE=$(( $VBITRATE * 106 / 100 ))
VBITRATE=$(echo $VBITRATE*106/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "3/4" ]; then
#VBITRATE=$(( $VBITRATE * 107 / 100 ))
VBITRATE=$(echo $VBITRATE*107/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "5/6" ]; then
#VBITRATE=$(( $VBITRATE * 108 / 100 ))
VBITRATE=$(echo $VBITRATE*108/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "8/9" ]; then
#VBITRATE=$(( $VBITRATE * 108 / 100 ))
VBITRATE=$(echo $VBITRATE*108/100 | bc -l | cut -c 1-6)
fi
if [ "$FEC" == "9/10" ]; then
#VBITRATE=$(( $VBITRATE * 108 / 100 ))
VBITRATE=$(echo $VBITRATE*109/100 | bc -l | cut -c 1-6)
fi

fi

	fi



if [ "$FW" = "yes" ] && [ "$AUTO" = "5" ]; then

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

	fi
	


if [ "$FW" = "yes" ] && [ "$AUTO" = "5" ] && [ "$DATVOUT" = "on" ]; then


# FREQUENCYRX
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


# RX-SR
cat ./ini/longmynd-sr.ini
echo "Please, choose your RX-SR 0-9 :"
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




		if [[ "$AUTO" == 6 ]]; then

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

# FREQUENCYRX
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
	

# RX-SR
cat ./ini/longmynd-sr.ini
echo "Please, choose your RX-SR 0-9 :"
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



# Check parameters
if (( $(bc <<<"$SR > 4000") )); then
echo "Invalid SR >4000KS"
exit
fi
if (( $(bc <<<"$VBITRATE < 20") )); then
echo "Invalid parameters, Video-Bitrate below 20K"
exit
fi
if (( $(bc <<<"$VBITRATE < 50 && $FPS > 30") )); then
echo "echo Invalid parameters, FPS too high"
exit
fi


#Write to favorite-x.ini
if [ "$CHANGE" == "1" ]; then

#PRINT
echo -----------------------------------
echo Setting favorite parameters....
echo -----------------------------------

echo SR=$SR > ./ini/favorite-"$PROFILECOUNT".ini
echo MODE=$MODE >> ./ini/favorite-"$PROFILECOUNT".ini
echo FEC=$FEC >> ./ini/favorite-"$PROFILECOUNT".ini
echo IMAGESIZE=$IMAGESIZE >> ./ini/favorite-"$PROFILECOUNT".ini
echo FPS=$FPS >> ./ini/favorite-"$PROFILECOUNT".ini
echo AUDIO=$AUDIO >> ./ini/favorite-"$PROFILECOUNT".ini
echo CODEC=$CODEC >> ./ini/favorite-"$PROFILECOUNT".ini
echo TSBITRATE=$TSBITRATE >> ./ini/favorite-"$PROFILECOUNT".ini
echo VBITRATE=$VBITRATE >> ./ini/favorite-"$PROFILECOUNT".ini
echo ABITRATE=$ABITRATE >> ./ini/favorite-"$PROFILECOUNT".ini

echo TXFREQUENCY=$TXFREQUENCY >> ./ini/favorite-"$PROFILECOUNT".ini
echo RXFREQUENCY=$RXFREQUENCY >> ./ini/favorite-"$PROFILECOUNT".ini
echo RXSR=$RXSR >> ./ini/favorite-"$PROFILECOUNT".ini
echo GAIN=$GAIN >> ./ini/favorite-"$PROFILECOUNT".ini
fi

#GSE
# Write to favorite-x.ini
if [ "$CHANGE" == "2" ]; then

echo -----------------------------------
echo Setting favorite parameters....
echo -----------------------------------

echo GSE=1 > ./ini/favorite-"$PROFILECOUNT".ini
echo TXFREQUENCY=$TXFREQUENCY >> ./ini/favorite-"$PROFILECOUNT".ini
echo RXFREQUENCY=$RXFREQUENCY >> ./ini/favorite-"$PROFILECOUNT".ini
echo GAIN=$GAIN >> ./ini/favorite-"$PROFILECOUNT".ini
echo SR=$SR >> ./ini/favorite-"$PROFILECOUNT".ini
echo RXSR=$RXSR >> ./ini/favorite-"$PROFILECOUNT".ini
echo MODE=$MODE >> ./ini/favorite-"$PROFILECOUNT".ini
echo FEC=$FEC >> ./ini/favorite-"$PROFILECOUNT".ini
fi


echo --------------------------------------
echo Profile $PROFILECOUNT parameters are set
echo --------------------------------------

echo "Do you want to change another profile? YES (1), NO (2) :"
read CHANGEMORE

if [[ "$CHANGEMORE" == 2 ]]; then
exit
fi

done

