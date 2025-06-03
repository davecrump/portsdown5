#!/bin/bash

# set -x # Uncomment for testing

############# MAKE SURE THAT WE KNOW WHERE WE ARE ##################

cd /home/pi

# ########################## KILL ALL PROCESSES BEFORE STARTING  ################

#sudo killall -9 ffmpeg >/dev/null 2>/dev/null
#sudo killall portsdown >/dev/null 2>/dev/null

# Kill netcat that night have been started for Express Server
#sudo killall netcat >/dev/null 2>/dev/null
#sudo killall -9 netcat >/dev/null 2>/dev/null

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
VIDEOSOURCE=$(get_config_var videosource $PCONFIGFILE)
AUDIO_PREF=$(get_config_var audio $PCONFIGFILE)

AUDIO_CHANNELS=0

######################### Calculate the output frequency in Hz ###############

# Round down to 1 kHz resolution.  Answer is always integer numeric

FREQOUTPUTKHZ=`echo - | awk '{print sprintf("%d", '$FREQOUTPUT' * 1000)}'`
FREQOUTPUTHZ="$FREQOUTPUTKHZ"000

######################### Calculate the SR per second ###############

SYMBOLRATEPS="$SYMBOLRATE"000

# Make sure Lime gain is sensible
#if [ "$LIME_GAIN" -lt 6 ]; then
#  LIMEGAIN=6
#fi

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

#    let BITRATE_VIDEO=(BITRATE_TS*75)/100-10000
    let BITRATE_VIDEO=(BITRATE_TS*60)/100-10000
echo BITRATE_TS:
echo $BITRATE_TS
echo BITRATE_VIDEO:
echo $BITRATE_VIDEO
echo VIDEOSOURCE:
echo $VIDEOSOURCE

detect_audio    # returns audio parameters

printf "AUDIO_CARD = $AUDIO_CARD\n"
printf "AUDIO_CARD_NUMBER = $AUDIO_CARD_NUMBER \n"
printf "AUDIO_CHANNELS = $AUDIO_CHANNELS \n"
printf "AUDIO_SAMPLE = $AUDIO_SAMPLE \n"

detect_webcam_type # returns webcam type
echo WEBCAM_TYPE:
echo $WEBCAM_TYPE

detect_video       # returns /dev address
echo VID_WEBCAM:
echo $VID_WEBCAM


#exit

case "$VIDEOSOURCE" in

  "WebCam")

        INPUT_FORMAT="h264"
        AUDIO_SAMPLE=32000
        AUDIO_CHANNELS=2
        if [ "$FORMAT" == "16:9" ]; then
          VIDEO_WIDTH=800
          VIDEO_HEIGHT=448
        elif [ "$FORMAT" == "720p" ] || [ "$FORMAT" == "1080p" ]; then
          VIDEO_WIDTH=1280
          VIDEO_HEIGHT=720
        else
          VIDEO_WIDTH=800
          VIDEO_HEIGHT=600
        fi


#            v4l2-ctl --device=/dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=1 --set-parm=15 \
#              --set-ctrl power_line_frequency=1
            VIDEO_WIDTH=1280
            VIDEO_HEIGHT=720
            VIDEO_FPS=15
       #INPUT_FORMAT="mjpeg"


#        -i $VID_WEBCAM \

        ffmpeg \
        -thread_queue_size 2048 \
        -f v4l2 -input_format $INPUT_FORMAT -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" \
        -i $VID_WEBCAM \
        -f alsa -thread_queue_size 2048 -ac $AUDIO_CHANNELS -ar $AUDIO_SAMPLE \
        -i hw:$AUDIO_CARD_NUMBER,0 \
        -c:v libx264 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -g 25 \
        -c:a aac -ar 22050 -ac $AUDIO_CHANNELS -ab 64k \
            -f mpegts  -blocksize 1880 \
            -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 \
            -mpegts_service_id 1 \
            -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 \
            -metadata service_provider="Portsdown 5" -metadata service_name="G8GKQ" \
            -muxrate $BITRATE_TS -y "udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1" &
  ;;
  "TestCard")
          ffmpeg \
            -thread_queue_size 512 \
            -re -loop 1 \
            -framerate 5 -video_size 704x576 \
            -i $IMAGEFILE \
            \
            -vf fps=10 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v $BITRATE_VIDEO \
            -f mpegts  -blocksize 1880 \
            -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 \
            -mpegts_service_id 1 \
            -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 \
            -metadata service_provider="Portsdown 5" -metadata service_name="G8GKQ" \
            -muxrate $BITRATE_TS -y udp://127.0.0.1:10000 &
  ;;

esac