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

STREAM_URL="rtmp://rtmp.batc.org.uk/live"
STREAM_KEY=$(get_config_var streamkey $PCONFIGFILE)


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
#    let BITRATE_VIDEO=(BITRATE_TS*55)/100-10000
echo BITRATE_TS:
echo $BITRATE_TS
let BITRATE_TS=(BITRATE_TS*19)/20
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
echo VIDEOSOURCE:
echo $VIDEOSOURCE


############################## Code for BATC Streamer ######################################

    # To Set the sound/video lipsync
    # If sound arrives first, decrease the value to delay it
    # like "-00:00:0.2" 


if [ "$MODEOUTPUT" == "STREAMER" ]; then

  if [ "$VIDEOSOURCE" == "ATEMUSB" ]; then

    ITS_OFFSET="-00:00:0.3"

    ffmpeg -thread_queue_size 2048 \
           -itsoffset "$ITS_OFFSET" \
           -f v4l2 \
           -i $VID_WEBCAM \
           -f alsa -thread_queue_size 2048 -ac 2 -ar 48000 \
           -i hw:$AUDIO_CARD_NUMBER,0 \
           -c:v libx264 -b:v 750k -r 15 -g 100  \
           -c:a aac -ar 22050 -ac 2 -ab 64k \
           -f flv $STREAM_URL/$STREAM_KEY &
  fi

  if [ "$VIDEOSOURCE" == "CAMLINK4K" ]; then

    ITS_OFFSET="-00:00:0.3"

    ffmpeg -thread_queue_size 2048 \
           -itsoffset "$ITS_OFFSET" \
           -f v4l2 \
           -i $VID_WEBCAM \
           -f alsa -thread_queue_size 2048 -ac 2 -ar 48000 \
           -i hw:$AUDIO_CARD_NUMBER,0 \
           -c:v libx264 -b:v 750k -r 15 -g 100  \
           -c:a aac -ar 22050 -ac 2 -ab 64k \
           -f flv $STREAM_URL/$STREAM_KEY &
    fi

  if [ "$VIDEOSOURCE" == "WebCam" ]; then

    if [ "$WEBCAM_TYPE" == "OldC920" ]; then
      VIDEO_WIDTH=1920
      VIDEO_HEIGHT=1080
      INPUT_FORMAT="h264"
      ITS_OFFSET="-00:00:0.5"
    elif [ "$WEBCAM_TYPE" == "C170" ]; then
      VIDEO_WIDTH=1024
      VIDEO_HEIGHT=768
      ITS_OFFSET="-00:00:0.3"
      INPUT_FORMAT="mjpeg"
      v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width=1024,height=768,pixelformat=1 --set-parm=15 \
        --set-ctrl power_line_frequency=1
    elif [ "$WEBCAM_TYPE" == "EagleEye" ]; then
      VIDEO_WIDTH=1920
      VIDEO_HEIGHT=1080
      ITS_OFFSET="-00:00:0.15"
      INPUT_FORMAT="mjpeg"
      v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width=1920,height=1080,pixelformat=1 --set-parm=15 \
        --set-ctrl power_line_frequency=1
    else
      VIDEO_WIDTH=1920
      VIDEO_HEIGHT=1080
      ITS_OFFSET="-00:00:0.15"
      INPUT_FORMAT="mjpeg"
      v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width=1920,height=1080,pixelformat=1 --set-parm=15 \
        --set-ctrl power_line_frequency=1
    fi


    ffmpeg -thread_queue_size 2048 \
           -itsoffset "$ITS_OFFSET" \
           -f v4l2 -input_format $INPUT_FORMAT -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" \
           -i $VID_WEBCAM \
           -f alsa -thread_queue_size 2048 -ac $AUDIO_CHANNELS -ar 48000 \
           -i plughw:$AUDIO_CARD_NUMBER,0 \
           -c:v libx264 -b:v 750k -r 15 -g 100  -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" \
           -c:a aac -ar 22050 -ac $AUDIO_CHANNELS -ab 64k \
           -f flv $STREAM_URL/$STREAM_KEY &
  fi
  exit
fi

############################## Code for Lime Mini ######################################

    # To Set the sound/video lipsync
    # If sound arrives first, decrease the value to delay it
    # like "-00:00:0.2" 


if [ "$MODEOUTPUT" == "LIMEMINI" ]; then

  case "$VIDEOSOURCE" in

    "WebCam")

      if [ "$FORMAT" == "4:3" ]; then
        VIDEO_WIDTH=768
        VIDEO_HEIGHT=576
      elif [ "$FORMAT" == "16:9" ]; then
        VIDEO_WIDTH=800
        VIDEO_HEIGHT=448
      elif [ "$FORMAT" == "720p" ]; then
        VIDEO_WIDTH=1280
        VIDEO_HEIGHT=720
      elif [ "$FORMAT" == "1080p" ]; then
        VIDEO_WIDTH=1920
        VIDEO_HEIGHT=1080
      fi

      if [ "$WEBCAM_TYPE" == "OldC920" ]; then
        VIDEO_WIDTH=1920
        VIDEO_HEIGHT=1080
        INPUT_FORMAT="h264"
        ITS_OFFSET="-00:00:0.5"
      elif [ "$WEBCAM_TYPE" == "C170" ]; then
        VIDEO_WIDTH=1024
        VIDEO_HEIGHT=768
        ITS_OFFSET="-00:00:0.3"
        INPUT_FORMAT="mjpeg"
        v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width=1024,height=768,pixelformat=1 --set-parm=15 \
          --set-ctrl power_line_frequency=1
      elif [ "$WEBCAM_TYPE" == "EagleEye" ]; then
        ITS_OFFSET="-00:00:0.15"
        INPUT_FORMAT="mjpeg"
        v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width="$VIDEO_WIDTH",height="$VIDEO_HEIGHT",pixelformat=1 --set-parm=15 \
                --set-ctrl power_line_frequency=1
      else
        VIDEO_WIDTH=1920
        VIDEO_HEIGHT=1080
        ITS_OFFSET="-00:00:0.15"
        INPUT_FORMAT="mjpeg"
        v4l2-ctl --device="$VID_WEBCAM" --set-fmt-video=width=1920,height=1080,pixelformat=1 --set-parm=15 \
          --set-ctrl power_line_frequency=1
      fi



      ffmpeg \
        -thread_queue_size 2048 \
        -f v4l2 -input_format $INPUT_FORMAT -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" \
        -i $VID_WEBCAM \
        -f alsa -thread_queue_size 2048 -ac $AUDIO_CHANNELS -ar $AUDIO_SAMPLE \
        -i plughw:$AUDIO_CARD_NUMBER,0 \
        -c:v libx264 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -r 15 -g 100 \
        -c:a aac -ar 22050 -ac $AUDIO_CHANNELS -ab 64k \
            -f mpegts  -blocksize 1880 \
            -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 \
            -mpegts_service_id 1 \
            -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 \
            -metadata service_provider="Portsdown 5" -metadata service_name="G8GKQ" \
            -muxrate $BITRATE_TS -y "udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1" &

    ;;
  
    "EasyCap")

      if [ "$FORMAT" == "4:3" ]; then
        SCALE=""
      else
        SCALE="scale=1024:576,"
      fi

      CAPTION=" "


      # If a Fushicai EasyCap, adjust the contrast to prevent white crushing
      # Default is 464 (scale 0 - 1023) which crushes whites
      lsusb | grep -q '1b71:3002'
      if [ $? == 0 ]; then   ## Fushuicai USBTV007
        ECCONTRAST="contrast=380"
        # Set to PAL
        v4l2-ctl -d $VID_USB --set-standard=6
        INPUT_FORMAT="yuyv422"
      else
        ECCONTRAST=" "
      fi

      ffmpeg \
        -thread_queue_size 2048 \
        -f v4l2 -input_format $INPUT_FORMAT \
        -i $VID_USB  \
        -f alsa -thread_queue_size 2048 -ac $AUDIO_CHANNELS -ar $AUDIO_SAMPLE \
        -i plughw:$AUDIO_CARD_NUMBER,0 \
        -c:v libx264 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -bufsize:v 200000  \
        -vf "$CAPTION""$SCALE"yadif=0:1:0 -r 15 -g 100 \
        -c:a aac -ar 22050 -ac $AUDIO_CHANNELS -ab 64k \
            -f mpegts  -blocksize 1880 \
            -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 \
            -mpegts_service_id 1 \
            -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 \
            -metadata service_provider="Portsdown 5" -metadata service_name="G8GKQ" \
            -muxrate $BITRATE_TS -y "udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1" &

          # Set the EasyCap contrast to prevent crushed whites
          sleep 1
          v4l2-ctl -d "$VID_USB" --set-ctrl "$ECCONTRAST" >/dev/null 2>/dev/null
    ;;
    esac
  fi
exit

#-input_format $INPUT_FORMAT -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT"

ffmpeg -thread_queue_size 20480 -f v4l2 -i /dev/video0 -f alsa -thread_queue_size 20480 -ac 2 -ar 48000 -i hw:0,0 -c:v libx264 -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v 600000 -minrate:v 600000 -maxrate:v 600000 -bufsize:v 409600 -r 15 -g 100 -c:a aac -ar 22050 -ac 2 -ab 64k -f mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 1 -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 -metadata 'service_provider=Portsdown 5' -metadata service_name=G8GKQ -muxrate 420000 -y 'udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1' &

exit



ffmpeg -thread_queue_size 20480 -f v4l2 -i /dev/video0 -f alsa -thread_queue_size 20480 -ac 2 -ar 48000 -i hw:2,0 -c:v libx264 -b:v 254186 -minrate:v 254186 -maxrate:v 254186 -bufsize:v 409600 -r 15 -g 100 -c:a aac -ar 22050 -ac 2 -ab 64k -f mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 1 -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 -metadata 'service_provider=Portsdown 5' -metadata service_name=G8GKQ -muxrate 420000 -y 'udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1' &

exit

# This doesn't work
ffmpeg -thread_queue_size 20480 -f v4l2 -i /dev/video0 -f alsa -thread_queue_size 20480 -ac 2 -ar 48000 -i hw:2,0 -c:v libx264 -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v 600000 -minrate:v 600000 -maxrate:v 600000 -bufsize:v 409600 -r 15 -g 100 -c:a aac -ar 22050 -ac 2 -ab 64k \
  -f tee \
  "[mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 1 -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 -metadata 'service_provider=Portsdown 5' -metadata service_name=G8GKQ -muxrate 420000 -y udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1 \
  |mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 1 -mpegts_pmt_start_pid 4095 -streamid 0:256 -streamid 1:257 -metadata 'service_provider=Portsdown 5' -metadata service_name=G8GKQ -muxrate 420000 -y udp://127.0.0.1:10001?pkt_size=1316&overrun_nonfatal=1]" &

#-f tee "[f=flv:onfail=ignore]rtmp://localhost/stream/output_stream0|[f=flv:onfail=ignore]rtmp://localhost/stream/output_stream1"


# 'udp://127.0.0.1:10000?pkt_size=1316&overrun_nonfatal=1' -video_size 1280x720 -g 25 -muxrate 440310

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