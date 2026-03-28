#!/bin/bash
#exit
# set -x # Uncomment for testing

############# MAKE SURE THAT WE KNOW WHERE WE ARE ##################

cd /home/pi

# ########################## KILL ALL PROCESSES BEFORE STARTING  ################

sudo killall -9 ffmpeg >/dev/null 2>/dev/null
sudo killall portsdown >/dev/null 2>/dev/null

# Kill netcat
sudo killall netcat >/dev/null 2>/dev/null
sudo killall -9 netcat >/dev/null 2>/dev/null

############# DEFINE THE FUNCTIONS USED BELOW ##################

source /home/pi/portsdown/scripts/transmit/tx_functions.sh

############# SET GLOBAL VARIABLES ####################

PATHBIN="/home/pi/portsdown/bin"
PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"

IMAGEFILE="/home/pi/portsdown/images/tcf.jpg"

echo $PCONFIGFILE

MODULATION=$(get_config_var modulation $PCONFIGFILE)
ENCODING=$(get_config_var encoding $PCONFIGFILE)
MODEOUTPUT=$(get_config_var modeoutput $PCONFIGFILE)
FORMAT=$(get_config_var format $PCONFIGFILE)
VIDEOSOURCE=$(get_config_var videosource $PCONFIGFILE)
FREQOUTPUT=$(get_config_var freqoutput $PCONFIGFILE)
SYMBOLRATE=$(get_config_var symbolrate $PCONFIGFILE)
FEC=$(get_config_var fec $PCONFIGFILE)
LIMEGAIN=$(get_config_var limegain $PCONFIGFILE)
REQ_UPSAMPLE=$(get_config_var upsample $PCONFIGFILE)

PLUTOPWR="-10"


AUDIO_CHANNELS=0

if [ "$MODEOUTPUT" == "STREAMER" ]; then
  exit
fi


######################### Calculate the output frequency in Hz ###############

# Round down to 1 kHz resolution.  Answer is always integer numeric

FREQOUTPUTKHZ=`echo - | awk '{print sprintf("%d", '$FREQOUTPUT' * 1000)}'`
FREQOUTPUTHZ="$FREQOUTPUTKHZ"000

######################### Calculate the SR ###############

SYMBOLRATEPS="$SYMBOLRATE"000

# Make sure Lime gain is sensible
#if [ "$LIME_GAIN" -lt 6 ]; then
#  LIMEGAIN=6
#fi

let SYMBOLRATE_K=SYMBOLRATE/1000

LIME_GAINF=`echo - | awk '{print '$LIMEGAIN' / 100}'`

############# CALCULATE BITRATES ########################

case "$FEC" in
  "7" )
    FECNUM="7"
    FECDEN="8"
    PFEC="78"
  ;;
  "14" )
    FECNUM="1"
    FECDEN="4"
    PFEC="14"
  ;;
  "13" )
    FECNUM="1"
    FECDEN="3"
    PFEC="13"
  ;;
  "1" | "12" )
    FECNUM="1"
    FECDEN="2"
    PFEC="12"
  ;;
  "35" )
    FECNUM="3"
    FECDEN="5"
    PFEC="35"
  ;;
  "2" | "23" )
    FECNUM="2"
    FECDEN="3"
    PFEC="23"
  ;;
  "3" | "34" )
    FECNUM="3"
    FECDEN="4"
    PFEC="34"
  ;;
  "5" | "56" )
    FECNUM="5"
    FECDEN="6"
    PFEC="56"
  ;;
  "89" )
    FECNUM="8"
    FECDEN="9"
    PFEC="89"
  ;;
  "91" )
    FECNUM="9"
    FECDEN="10"
    PFEC="910"
  ;;
esac

case "$MODULATION" in
  "DVB-S" )
    BITSPERSYMBOL="2"
    MODTYPE="DVBS"
    CONSTLN="QPSK"
  ;;
  "S2QPSK" )
    BITSPERSYMBOL="2"
    MODTYPE="DVBS2"
    CONSTLN="QPSK"
  ;;
  "8PSK" )
    BITSPERSYMBOL="3"
    MODTYPE="DVBS2"
    CONSTLN="8PSK"
  ;;
  "16APSK" )
    BITSPERSYMBOL="4"
    MODTYPE="DVBS2"
    CONSTLN="16APSK"
  ;;
  "32APSK" )
    BITSPERSYMBOL="5"
    MODTYPE="DVBS2"
    CONSTLN="32APSK"
  ;;
esac

# Select pilots on if required
if [ "$PILOT" == "on" ]; then
  PILOTS="-p"
else
  PILOTS=""
fi

# Select Short Frames if required
if [ "$FRAME" == "short" ]; then
  FRAMES="-v"
else
  FRAMES=""
fi

########### Calculate the max bitrate ##############################

BITRATE_TS="$($PATHBIN"/dvb2iq" -s $SYMBOLRATE -f $FECNUM"/"$FECDEN \
              -d -r 1 -m $MODTYPE -c $CONSTLN $PILOTS $FRAMES )"

# Calculate the Video Bitrate for MPEG-2 Sound/no sound
if [ "$ENCODING" == "MPEG-2" ]; then
  if [ "$AUDIO_CHANNELS" != 0 ]; then                 # MPEG-2 with Audio
    let BITRATE_VIDEO=(BITRATE_TS*75)/100-74000
  else                                                # MPEG-2 no audio
    let BITRATE_VIDEO=(BITRATE_TS*75)/100-10000
  fi
fi

    let BITRATE_VIDEO=(BITRATE_TS*75)/100-10000

# Set the upsampling rate
   if [ "$SYMBOLRATE_K" -lt 990 ] ; then
      UPSAMPLE=2
     elif [ "$SYMBOLRATE_K" -lt 1500 ] && [ "$MODULATION" == "DVB-S" ] ; then
      UPSAMPLE=2
    else
      UPSAMPLE=1
    fi

################ Set up for PLUTO and LIBRESDR ###################

case "$MODEOUTPUT" in

  PLUTO)
    PLUTOPWR=$(get_config_var plutopwr $PCONFIGFILE)

    # Add an extra / to the beginning of the Pluto call if required to cope with /P
    echo $CALL | grep -q "/"
    if [ $? == 0 ]; then
      PLUTOCALL="/,${CALL}"
    else
      PLUTOCALL=",${CALL}"
    fi
  ;;

  LIBRESDR)
    PLUTOPWR=$(get_config_var plutopwr $PCONFIGFILE)

    # Add an extra / to the beginning of the Pluto call if required to cope with /P
    echo $CALL | grep -q "/"
    if [ $? == 0 ]; then
      PLUTOCALL="/,${CALL}"
    else
      PLUTOCALL=",${CALL}"
    fi

    # Set the LibreSDR IP Address
    if [ "$LIBREIP" == "dhcp" ]; then
      PLUTOIP=libre
    else
      PLUTOIP=$LIBREIP
    fi

echo In Pluto

    MODEOUTPUT=PLUTO
  ;;
esac

sudo rm videots >/dev/null 2>/dev/null
mkfifo videots

##################### Transmit TS for each ouput device ###################################

##################### Lime Mini ###########################################################

if [ "$MODEOUTPUT" == "LIMEMINI" ]; then

  $PATHBIN/"limesdr_dvb" -i videots -s $SYMBOLRATEPS -f $FECNUM"/"$FECDEN -r $UPSAMPLE -m DVBS2 -c $CONSTLN -t $FREQOUTPUTHZ -g $LIME_GAINF -q 1 -D 27 -e 2  >/dev/null 2>/dev/null &

  netcat -u -4 -l 10000 > videots

##################### Lime using LimeSuiteNG ##############################################

elif [ "$MODEOUTPUT" == "LIMEXTRX" ] || [ "$MODEOUTPUT" == "LIMEMINING" ]; then

  $PATHBIN/"limesdr_dvbng" -i videots -s $SYMBOLRATEPS -f $FECNUM"/"$FECDEN -r 1 -m $MODTYPE -c QPSK -t $FREQOUTPUTHZ -g $LIME_GAINF -q 1 -D 27 -e 2 &

  #$PATHBIN/"limesdr_dvbng" -i videots -s $SYMBOLRATEPS -f carrier -r 1 -m DVBS2 -c QPSK -t $FREQOUTPUTHZ -g $LIME_GAINF -q 1 -D 27 -e 2 &

  netcat -u -4 -l 10000 > videots

###################### PLUTO and LIBRESDR #################################################

elif [ "$MODEOUTPUT" == "PLUTO" ]; then

echo in pluto 2

echo rtmp://$PLUTOIP:7272/,$FREQOUTPUT,$MODTYPE,$CONSTLN,$SYMBOLRATE_K,$PFEC,-$PLUTOPWR,nocalib,800,32,/$PLUTOCALL,

  ffmpeg -thread_queue_size 2048 \
    -i udp://:@:10000?fifo_size=1000000"&"overrun_nonfatal=1 -c:v copy -c:a copy \
    -f flv \
    rtmp://$PLUTOIP:7272/,$FREQOUTPUT,$MODTYPE,$CONSTLN,$SYMBOLRATE_K,$PFEC,-$PLUTOPWR,nocalib,800,32,/$PLUTOCALL, &

fi


exit

