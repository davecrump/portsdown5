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

# ###################################################

cat ./version.txt
echo Running script in $BASEDIR

# You can adjust the lines below when you know what you are doing ;-)
# ###################################################
# Read previous run from params.ini and favorites from favorite-x.ini
# If programm was not cleanly closed
echo RXL=off > ./ini/rxonoff.ini
echo RELAY=off > ./ini/relay.ini

echo -------------------------
echo Last run:
cat ./ini/params.ini
echo -------------------------
echo -------------------------
echo Profile 1 setup:
cat ./ini/favorite-1.ini
echo -------------------------
echo -------------------------
echo Profile 2 setup:
cat ./ini/favorite-2.ini
echo -------------------------
echo -------------------------
echo Profile 3 setup:
cat ./ini/favorite-3.ini
echo -------------------------

echo "Use profile 1 (1), profile 2 (2), profile 3 (3), use previous parameters (4), start new parameters (5), GSE-Mode (6) :"
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

# Videobitrate calculation
# Up to 35K
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
#VBITRATE=$(( 1000 * $TSBITRATE * 50 / 100 / 1000 - $ABITRATE))
VBITRATE=$(echo $TSBITRATE*50/100-$ABITRATE | bc -l | cut -c 1-6)
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


#Separation due to autorun
	if [[ "$AUTO" == 6 ]] || [[ "$GSE" == "1" ]]; then


# GSE RX
LONGFREQUENCY=$(( $RXFREQUENCY - $RXOFFSET * 1000 ))
mosquitto_pub -t $CMD_ROOT/system/longmynd -m on -h $PLUTOIP
echo "Wait 10s for Longmynd to start..."
sleep 10
mosquitto_pub -t cmd/longmynd/frequency -m $LONGFREQUENCY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/sr -m $RXSR -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/tsip -m $TSIP -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/rxbbframeip -m $MCAST:$MCASTPORT -h $PLUTOIP
echo RXL=on > ./ini/rxonoff.ini

# GSE TX
TXMODE=dvbs2-gse
FRAME=short

# GSE Routing
mosquitto_pub -t $CMD_ROOT/ip/tunadress -m $TUNIP -h $PLUTOIP
# if "$ROUTE"=="yes" (route add $ROUTENET $PLUTOIP)
# if "$ROUTE"=="yes" start "ROUTING" ./scripts/ROUTING.BAT

# GSE init iptables
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-t nat -F" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 >> /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-F" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 > /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-A FORWARD -p udp -o gse0 -s $NETWORK -j DROP" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 > /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-t nat -A POSTROUTING -o gse0  -j MASQUERADE" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 > /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-t nat -A POSTROUTING -o eth0  -j MASQUERADE" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 > /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-t nat -A PREROUTING -p udp -i gse0 --dport $PORTSTART:$PORTEND -j DNAT --to-destination $PCFORWARD1:$PORTSTART-$PORTEND" -h $PLUTOIP
ping -c 1 -w 10 127.255.255.255 > /dev/null 2>&1
mosquitto_pub -t $CMD_ROOT/ip/iptables -m "-t nat -A PREROUTING -p tcp -i gse0 --dport $PORT -j DNAT --to-destination $PCFORWARD2:$PORT" -h $PLUTOIP

# GSE Status and info
echo "GSE-Mode activated, frame type (short) and txmode (dvbs2-gse) automatically set:"
echo "######################################################################"
echo TX-Frequency: "$TXFREQUENCY"Mhz
echo RX-Frequency: "$RXFREQUENCY"Mhz
echo Offset-Frequency: $RXOFFSET Mhz
echo Longmynd-Frequency: "$LONGFREQUENCY"Khz
echo Gain: $GAIN
echo TX-SR: "$SR"KS
echo RX-SR: "$RXSR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Mode type: $TXMODE
echo Frame type: $FRAME
echo Status Longmynd: on
echo "######################################################################"
echo "To set up the routing for the local PC, you have to start /scripts/ROUTING.sh"

# # Write run to params.ini
echo GSE=1 > ./ini/params.ini
echo TXFREQUENCY=$TXFREQUENCY >> ./ini/params.ini
echo RXFREQUENCY=$RXFREQUENCY >> ./ini/params.ini
echo GAIN=$GAIN >> ./ini/params.ini
echo SR=$SR >> ./ini/params.ini
echo RXSR=$RXSR >> ./ini/params.ini
echo MODE=$MODE >> ./ini/params.ini
echo FEC=$FEC >> ./ini/params.ini

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
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/gainvariable -m $GAINVARIABLE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/digitalgain -m $DIGITALGAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecrange -m $FECRANGE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcemode -m $TSSOURCEMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcefile -m $TSSOURCEFILE -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/lnb_supply -m $LNBSUPPLY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/polarisation -m $LNBPOL -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/swport -m $TUNERPORT -h $PLUTOIP

# Start Control and MQTT Browser
xfce4-terminal --title DATV-NotSoEasy-CONTROL -e 'bash ./scripts/CONTROL.sh' &
mqtt-explorer >> /dev/null 2>&1 &

# Running GSE or till interaction
echo "GSE-Mode started, press enter to exit..."
read

# Kill for GSE-Mode
mosquitto_pub -t $CMD_ROOT/system/longmynd -m off -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
if [[ "$REBOOT" == "on" ]]; then
mosquitto_pub -t $CMD_ROOT/system/reboot -m 1 -h $PLUTOIP
fi
killall mqtt-explorer
killall xfce4-terminal
# if [[ "$ROUTE" == yes ]]; then
# route delete $ROUTENET
# fi
echo RXL=off > ./ini/rxonoff.ini
exit

	fi



#DATVRX

# DATV-RELAY Mode
if [ "$FW" == "yes" ] && [ "$RELAY" == "on" ]; then
PLUTOPORT=1234
DATVOUTIP=$PLUTOIP
FECMODE=variable
DATVOUT=on
fi

# DATV RX
	if [ "$FW" == "yes" ] && [ "$DATVOUT" == "on" ]; then
LONGFREQUENCY=$(( $RXFREQUENCY - $RXOFFSET * 1000 ))
mosquitto_pub -t $CMD_ROOT/system/longmynd -m on -h $PLUTOIP
echo Wait 10s for Longmynd to start...
sleep 10
mosquitto_pub -t cmd/longmynd/frequency -m $LONGFREQUENCY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/sr -m $RXSR -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/tsip -m $DATVOUTIP -h $PLUTOIP
echo RXL=on > ./ini/rxonoff.ini

# DATV Status and info
echo "DATV-RX activated:"
echo "######################################################################"
echo TX-Frequency: "$TXFREQUENCY"Mhz
echo RX-Frequency: "$RXFREQUENCY"Mhz
echo Offset-Frequency: "$RXOFFSET"Mhz
echo Longmynd-Frequency: "$LONGFREQUENCY"Khz
echo Gain: $GAIN
echo TX-SR: "$SR"KS
echo RX-SR: "$RXSR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Mode type: $TXMODE
echo Frame type: $FRAME
echo Status Longmynd: on
echo "######################################################################"

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
mosquitto_pub -t $CMD_ROOT/tx/mute -m %MUTE% -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/frequency -m $TXFREQUENCY -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/sr -m $SRM -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/nco -m %NCO% -h $PLUTOIP
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
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/gainvariable -m $GAINVARIABLE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/digitalgain -m $DIGITALGAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecrange -m $FECRANGE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcemode -m $TSSOURCEMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcefile -m $TSSOURCEFILE -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/lnb_supply -m $LNBSUPPLY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/polarisation -m $LNBPOL -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/swport -m $TUNERPORT -h $PLUTOIP



if ! [ "$RELAY" = "on" ]; then
# Start decoder
xfce4-terminal --title DATV-NotSoEasy-CONTROL -e 'bash ./scripts/START-FFPLAY-LONGMYND.sh' &
fi

if [ "$RELAY" = "on" ]; then
# Running RELAY-Mode till interaction
xfce4-terminal --title DATV-NotSoEasy-CONTROL -e 'bash ./scripts/CONTROL.sh' &
mqtt-explorer >> /dev/null 2>&1 &
echo "RELAY-Mode started, press enter to exit..."
echo RELAY=on > ./ini/relay.ini

read

# Kill for RELAY-Mode
mosquitto_pub -t $CMD_ROOT/system/longmynd -m off -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
if ! [ "$REBOOT" = "on" ]; then
mosquitto_pub -t $CMD_ROOT/system/reboot -m 1 -h $PLUTOIP
fi
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
echo RXL=off > ./ini/rxonoff.ini
exit
fi

	fi



#MOSQUITTO
	if [ "$FW" == "yes" ]; then
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
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/gainvariable -m $GAINVARIABLE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/digitalgain -m $DIGITALGAIN -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/fecrange -m $FECRANGE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcemode -m $TSSOURCEMODE -h $PLUTOIP
mosquitto_pub -t $CMD_ROOT/tx/dvbs2/tssourcefile -m $TSSOURCEFILE -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/lnb_supply -m $LNBSUPPLY -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/polarisation -m $LNBPOL -h $PLUTOIP
mosquitto_pub -t cmd/longmynd/swport -m %TUNERPORT -h $PLUTOIP

# Start Control, decoder and MQTT Browser
xfce4-terminal --title DATV-NotSoEasy-CONTROL -e 'bash ./scripts/CONTROL.sh' &
mqtt-explorer >> /dev/null 2>&1 &

	fi



# Check parameters
if (( $(bc <<<"$SR > 4000") )); then
echo "Invalid SR >4000KS"
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
exit
fi

if (( $(bc <<<"$VBITRATE < 20") )); then
echo "Invalid parameters, Video-Bitrate below 20K"
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
exit
fi

if (( $(bc <<<"$VBITRATE < 50 && $FPS > 30") )); then
echo "echo Invalid parameters, FPS too high"
sleep 5
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
exit
fi


echo -----------------------------------
echo Running parameters:
echo -----------------------------------
echo Service-Name: $CALLSIGN
echo Service-Provider: $SERVICEPROVIDER
echo Pluto-IP: $PLUTOIP
echo Pluto-Port: $PLUTOPORT
echo TX-Frequency: $TXFREQUENCY
echo RX-Frequency: $RXFREQUENCY
echo Gain: $GAIN
echo TX-SR: "$SR"KS
echo RX-SR: "$RXSR"KS
echo TX-Mode: $MODE
echo TX-FEC: $FEC
echo Image size: $IMAGESIZE
echo FPS: $FPS
echo Audiochannels: $AUDIO
echo CODEC: $CODEC
echo TS-Bitrate: "$TSBITRATE"K
echo Video-Bitrate: "$VBITRATE"K
echo Audio-Bitrate: "$ABITRATE"K

# Write run to params.ini
echo SR=$SR > ./ini/params.ini
echo MODE=$MODE >> ./ini/params.ini
echo FEC=$FEC >> ./ini/params.ini
echo IMAGESIZE=$IMAGESIZE >> ./ini/params.ini
echo FPS=$FPS >> ./ini/params.ini
echo AUDIO=$AUDIO >> ./ini/params.ini
echo CODEC=$CODEC >> ./ini/params.ini
echo TSBITRATE=$TSBITRATE >> ./ini/params.ini
echo VBITRATE=$VBITRATE >> ./ini/params.ini
echo ABITRATE=$ABITRATE >> ./ini/params.ini

echo TXFREQUENCY=$TXFREQUENCY >> ./ini/params.ini
echo RXFREQUENCY=$RXFREQUENCY >> ./ini/params.ini
echo RXSR=$RXSR >> ./ini/params.ini
echo GAIN=$GAIN >> ./ini/params.ini



#HARDENC
	if [ "$VBR" = "off" ]; then

if [ "$CODEC" = "h264_nvenc" ] || [ "$CODEC" = "hevc_nvenc" ]; then

# Headroom for HW-Encoding
if (( $(bc <<<"$SR > 249 && $SR < 500") )); then
#VBITRATE=$(( $VBITRATE * 88 / 100 ))
VBITRATE=$(echo $VBITRATE*88/100 | bc -l | cut -c 1-6)
fi
if (( $(bc <<<"$SR > 20 && $SR < 250") )); then
#VBITRATE=$(( $VBITRATE * 70 / 100 ))
VBITRATE=$(echo $VBITRATE*70/100 | bc -l | cut -c 1-6)
fi

#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo ------------------------------------------
echo Hardware FFMPEG Encoder H.264/H.265
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate: "$VBITRATE"K
echo Audiobitrate: "$ABITRATE"K
echo Buffersize: "$BUFSIZE"K
echo Maxdelay: "$MAXDELAY"ms
echo Maxinterleave: "$MAXINTERLEAVE"s
echo ------------------------------------------


if [ "$INPUTTYPE" == "V4L2" ]; then

# Nvidia-Driver bug: -bf 0 switches of check of B-Frames
#
# Hardware encoder via V4L2
ffmpeg \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$VIDEODEVICE" -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ac $AUDIO -itsoffset $OFFSET \
-i "$AUDIODEVICE" -vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -b_ref_mode 0 -bf 0 -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K \
-rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -preset slow \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# Hardware encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -b_ref_mode 0 -bf 0 -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K \
-rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -preset slow \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# Hardware encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET \
-f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -b_ref_mode 0 -bf 0 -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K \
-rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -preset slow \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# Hardware encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -b_ref_mode 0 -bf 0 -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K \
-rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -preset slow \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi



#SOFTENC
if [ "$CODEC" = "libx264" ] || [ "$CODEC" = "libx265" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo ------------------------------------------
echo Software FFMPEG Encoder H.264/H.265
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate: "$VBITRATE"K
echo Audiobitrate: "$ABITRATE"K
echo Buffersize: "$BUFSIZE"K
echo Maxdelay: "$MAXDELAY"ms
echo Maxinterleave: "$MAXINTERLEAVE"s
echo ------------------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# Software encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$VIDEODEVICE" \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$AUDIODEVICE" -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# Software encoder via network UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# Software encoder via network RTMP
ffmpeg -itsoffset $OFFSET -f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# Software encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi




#VVC
if [ "$CODEC" = "libvvenc" ]; then

# Headroom for VVC-Encoding
if (( $(bc <<<"$SR > 249 && $SR < 500") )); then
#VBITRATE=$(( $VBITRATE * 80 / 100 ))
VBITRATE=$(echo $VBITRATE*80/100 | bc -l | cut -c 1-6)
fi
if (( $(bc <<<"$SR > 20 && $SR < 250") )); then
#VBITRATE=$(( $VBITRATE * 70 / 100 ))
VBITRATE=$(echo $VBITRATE*70/100 | bc -l | cut -c 1-6)
fi

#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi
# More stable
MAXDELAY=$(( $MAXDELAY * 10 ))

echo -----------------------------------
echo Warning! Experimental VVC Encoder
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate for VVC: "$VBITRATE"K
echo Audiobitrate for VVC: "$ABITRATE"K
echo Buffersize for VVC: "$BUFSIZE"K
echo Maxdelay for VVC: "$MAXDELAY"ms
echo Maxinterleave for VVC: "$MAXINTERLEAVE"s
echo -----------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# VVC encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$VIDEODEVICE" \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$AUDIODEVICE" -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi



if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# VVC encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# VVC encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET \
-f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M  \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# VVC encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi



#AV1
if [ "$CODEC" = "libaom-av1" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo -----------------------------------
echo Warning! Experimental AV1 Encoder
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate for AV1: "$VBITRATE"K
echo Audiobitrate for AV1: "$ABITRATE"K
echo Buffersize for AV1: "$BUFSIZE"K
echo Maxdelay for AV1: "$MAXDELAY"ms
echo Maxinterleave for AV1: "$MAXINTERLEAVE"s
echo -----------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# DVB-GSE container will be the best. Wait for implementation or modify source of ffmpeg to accept AV1 in mpegts
# -preset 8 -crf 30 -g 300 -usage realtime HDR -colorspace bt2020nc -color_trc smpte2084 -color_primaries bt2020 Test -svtav1-params tune=0 -svtav1-params fast-decode=1

# AV1 encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$VIDEODEVICE" \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$AUDIODEVICE" -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# AV1 encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# AV1 encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET \
-f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# AV1 encoder read file
ffmpeg -itsoffset $OFFSET -re -stream_loop $STREAMLOOP \
-i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -minrate "$VBITRATE"K -maxrate "$VBITRATE"K -bufsize "$BUFSIZE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -muxrate "$TSBITRATE"K -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi

	fi

fi




#HARDENC
	if [ "$VBR" = "on" ]; then
	
if [ "$CODEC" = "h264_nvenc" ] || [ "$CODEC" = "hevc_nvenc" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo ------------------------------------------
echo VBR-MODE !
echo Hardware FFMPEG Encoder H.264/H.265
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate: "$VBITRATE"K
echo Audiobitrate: "$ABITRATE"K
echo Buffersize: "$BUFSIZE"K
echo Maxdelay: "$MAXDELAY"ms
echo Maxinterleave: "$MAXINTERLEAVE"s
echo ------------------------------------------


if [ "$INPUTTYPE" == "V4L2" ]; then

# Nvidia-Driver bug: -bf 0 switches of check of B-Frames
#
# Hardware encoder via V4L2
ffmpeg \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$VIDEODEVICE" -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ac $AUDIO -itsoffset $OFFSET \
-i "$AUDIODEVICE" -vcodec $CODEC -s $IMAGESIZE -bf 0 -preset slow -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -rc-lookahead 10 -no-scenecut 1 -zerolatency 1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID \
-mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER \
-metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# Hardware encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K -vcodec $CODEC -s $IMAGESIZE -b_ref_mode 0 -bf 0 \
-b:v "$VBITRATE"K -r $FPS -g $KEYHARD -rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID \
-mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER \
-metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# Hardware encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET -f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -bf 0 -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -rc-lookahead 10 -no-scenecut 1 -zerolatency 1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID \
-mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER \
-metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# Hardware encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K -vcodec $CODEC -s $IMAGESIZE \
-b_ref_mode 0 -bf 0 -b:v "$VBITRATE"K -r $FPS -g $KEYHARD -rc-lookahead 10 -no-scenecut 1 -zerolatency 1 -acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -bufsize "$BUFSIZE"K \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID \
-mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER \
-metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi



#SOFTENC
if [ "$CODEC" = "libx264" ] || [ "$CODEC" = "libx265" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo ------------------------------------------
echo VBR-MODE !
echo Software FFMPEG Encoder H.264/H.265
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate: "$VBITRATE"K
echo Audiobitrate: "$ABITRATE"K
echo Buffersize: "$BUFSIZE"K
echo Maxdelay: "$MAXDELAY"ms
echo Maxinterleave: "$MAXINTERLEAVE"s
echo ------------------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# Software encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$VIDEODEVICE" -f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$AUDIODEVICE" -ar "$ABITRATE"K -vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYSOFT -acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M \
-pcr_period $PCRPERIOD -pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# Software encoder via network UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# Software encoder via network RTMP
ffmpeg -itsoffset $OFFSET -f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -b:v "$VBITRATE"K -r $FPS -g $KEYSOFT \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# Software encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K -vcodec $CODEC -s $IMAGESIZE \
-b:v "$VBITRATE"K -r $FPS -g $KEYSOFT -acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi



#VVC
if [ "$CODEC" = "libvvenc" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi
# More stable
MAXDELAY=$(( $MAXDELAY * 10 ))

echo -----------------------------------
echo VBR-MODE !
echo Warning! Experimental VVC Encoder
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate for VVC: "$VBITRATE"K
echo Audiobitrate for VVC: "$ABITRATE"K
echo Buffersize for VVC: "$BUFSIZE"K
echo Maxdelay for VVC: "$MAXDELAY"ms
echo Maxinterleave for VVC: "$MAXINTERLEAVE"s
echo -----------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# VVC encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$VIDEODEVICE" -f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i "$AUDIODEVICE" -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi



if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# VVC encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# VVC encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET \
-f flv -listen 1 -i $STREAMRTMP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi



if [ "$INPUTTYPE" == "FILE" ]; then

# VVC encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -preset faster -r $FPS -b:v "$VBITRATE"K -g $KEYVVC \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID \
-mpegts_pmt_start_pid $PMTPID -mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


fi



#AV1
if [ "$CODEC" = "libaom-av1" ]; then


#BUFSIZE=$(( $VBITRATE * $BUFFACTOR ))
BUFSIZE=$(echo $VBITRATE*$BUFFACTOR | bc -l | cut -c 1-6)
if (( $(bc <<<"$SR < 66") )); then
BUFSIZE=$(( $BUFSIZE * 3 ))
fi

# Avoid negative timestamps and DTS error
if (( $(bc <<<"$SR > 20 && $SR < 36") )); then
MAXINTERLEAVE=0
MAXDELAY=2000
fi

echo -----------------------------------
echo VBR-MODE !
echo Warning! Experimental AV1 Encoder
echo TX-Frequency: $TXFREQUENCY
echo Gain: $GAIN
echo SR: "$SR"KS
echo Mode: $MODE
echo FEC: $FEC
echo Codec: $CODEC
echo Resolution: $IMAGESIZE
echo FPS: $FPS
echo TS-Bitrate: "$TSBITRATE"K
echo Videobitrate for AV1: "$VBITRATE"K
echo Audiobitrate for AV1: "$ABITRATE"K
echo Buffersize for AV1: "$BUFSIZE"K
echo Maxdelay for AV1: "$MAXDELAY"ms
echo Maxinterleave for AV1: "$MAXINTERLEAVE"s
echo -----------------------------------

if [ "$INPUTTYPE" == "V4L2" ]; then

# DVB-GSE container will be the best. Wait for implementation or modify source of ffmpeg to accept AV1 in mpegts
# -preset 8 -crf 30 -g 300 -usage realtime HDR -colorspace bt2020nc -color_trc smpte2084 -color_primaries bt2020 Test -svtav1-params tune=0 -svtav1-params fast-decode=1

# AV1 encoder via V4L2
ffmpeg -itsoffset $OFFSET \
-f v4l2 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$VIDEODEVICE" \
-f alsa -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -i "$AUDIODEVICE" -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKUDP" ]; then

# AV1 encoder via NETWORK UDP
ffmpeg -overrun_nonfatal 1 -fifo_size "$FIFOBUF"M -itsoffset $OFFSET \
-i $STREAMUDP -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "NETWORKRTMP" ]; then

# AV1 encoder via NETWORK RTMP
ffmpeg -itsoffset $OFFSET \
-f flv -listen 1 -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M \
-i $STREAMRTMP -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi


if [ "$INPUTTYPE" == "FILE" ]; then

# AV1 encoder read file
ffmpeg -itsoffset $OFFSET \
-re -stream_loop $STREAMLOOP -i $STREAMFILE -thread_queue_size "$THREADQUEUE"K -rtbufsize "$RTBUF"M -ar "$ABITRATE"K \
-vcodec $CODEC -s $IMAGESIZE -crf $AV1QUAL -usage realtime -b:v "$VBITRATE"K -g $KEYAV1 \
-acodec $AUDIOCODEC -ac $AUDIO -b:a "$ABITRATE"K \
-f mpegts -streamid 0:$VIDEOPID -streamid 1:$AUDIOPID -max_delay "$MAXDELAY"K -max_interleave_delta "$MAXINTERLEAVE"M -pcr_period $PCRPERIOD \
-pat_period $PATPERIOD -mpegts_service_id $SERVICEID -mpegts_original_network_id $NETWORKID -mpegts_transport_stream_id $STREAMID -mpegts_pmt_start_pid $PMTPID \
-mpegts_start_pid $MPEGTSSTARTPID -metadata service_provider=$SERVICEPROVIDER -metadata service_name=$CALLSIGN -af aresample=async=1 \
"udp://"$PLUTOIP":"$PLUTOPORT"?pkt_size=1316"

if [ "$FW" = "yes" ]; then
mosquitto_pub -t $CMD_ROOT/tx/mute -m 1 -h $PLUTOIP
killall mqtt-explorer
killall xfce4-terminal
killall ffplay
fi

fi

	fi

fi

