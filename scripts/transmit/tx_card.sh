#!/bin/bash

# set -x # Uncomment for testing

# Called if test card is in use to format testcard and add caption


############# SET GLOBAL VARIABLES ####################

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"

TESTCARD=$(get_config_var testcard $PCONFIGFILE)

rm /home/pi/tmp/testcard.jpg >/dev/null 2>/dev/null
rm /home/pi/tmp/caption.png >/dev/null 2>/dev/null

if [ "$TESTCARD" == "tcf" ]; then

 if [ "$FORMAT" == "4:3" ]; then
    VIDEO_WIDTH=720
    VIDEO_HEIGHT=576

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 720x80 xc:transparent -fill white -gravity Center -pointsize 40 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/tcf.jpg /home/pi/tmp/caption.png -geometry +0+475 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/tcf.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "16:9" ]; then
    VIDEO_WIDTH=1024
    VIDEO_HEIGHT=576

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1024x80 xc:transparent -fill white -gravity Center -pointsize 50 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/tcfw16.jpg /home/pi/tmp/caption.png -geometry +0+475 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/tcfw16.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "720p" ]; then
    VIDEO_WIDTH=1280
    VIDEO_HEIGHT=720

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1280x80 xc:transparent -fill white -gravity Center -pointsize 65 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/tcfw.jpg /home/pi/tmp/caption.png -geometry +0+604 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/tcfw.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "1080p" ]; then
    VIDEO_WIDTH=1920
    VIDEO_HEIGHT=1080

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1920x100 xc:transparent -fill white -gravity Center -pointsize 100 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/tcf1080.jpg /home/pi/tmp/caption.png -geometry +0+917 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/tcf1080.jpg /home/pi/tmp/testcard.jpg
    fi

  fi

elif [ "$TESTCARD" == "tcc" ]; then

 if [ "$FORMAT" == "4:3" ]; then
    VIDEO_WIDTH=720
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/tcc.jpg /home/pi/tmp/testcard.jpg
 
  elif [ "$FORMAT" == "16:9" ]; then
    VIDEO_WIDTH=1024
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/tccw16.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "720p" ]; then
    VIDEO_WIDTH=1280
    VIDEO_HEIGHT=720

    cp /home/pi/portsdown/images/testcards/tccw.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "1080p" ]; then
    VIDEO_WIDTH=1920
    VIDEO_HEIGHT=1080

    cp /home/pi/portsdown/images/testcards/tccw.jpg /home/pi/tmp/testcard.jpg
  fi

elif [ "$TESTCARD" == "pm5544" ]; then

 if [ "$FORMAT" == "4:3" ]; then
    VIDEO_WIDTH=720
    VIDEO_HEIGHT=576

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 720x80 xc:transparent -fill white -gravity Center -pointsize 35 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/pm5544.jpg /home/pi/tmp/caption.png -geometry +0+423 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/pm5544.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "16:9" ]; then
    VIDEO_WIDTH=1024
    VIDEO_HEIGHT=576

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1024x80 xc:transparent -fill white -gravity Center -pointsize 35 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/pm5544w16.jpg /home/pi/tmp/caption.png -geometry +0+421 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/pm5544w16.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "720p" ]; then
    VIDEO_WIDTH=1280
    VIDEO_HEIGHT=720

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1280x80 xc:transparent -fill white -gravity Center -pointsize 45 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/pm5544w.jpg /home/pi/tmp/caption.png -geometry +0+536 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/pm5544w.jpg /home/pi/tmp/testcard.jpg
    fi

  elif [ "$FORMAT" == "1080p" ]; then
    VIDEO_WIDTH=1920
    VIDEO_HEIGHT=1080

    if [ "$CAPTIONON" == "on" ]; then
      convert -font "FreeSans" -size 1280x80 xc:transparent -fill white -gravity Center -pointsize 45 -annotate 0 $CALL /home/pi/tmp/caption.png
      convert /home/pi/portsdown/images/testcards/pm5544w.jpg /home/pi/tmp/caption.png -geometry +0+536 -composite /home/pi/tmp/testcard.jpg
    else
      cp /home/pi/portsdown/images/testcards/pm5544w.jpg /home/pi/tmp/testcard.jpg
    fi

  fi

elif [ "$TESTCARD" == "colourbars" ]; then

 if [ "$FORMAT" == "4:3" ]; then
    VIDEO_WIDTH=720
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/75cb.jpg /home/pi/tmp/testcard.jpg
 
  elif [ "$FORMAT" == "16:9" ]; then
    VIDEO_WIDTH=1024
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/75cbw16.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "720p" ]; then
    VIDEO_WIDTH=1280
    VIDEO_HEIGHT=720

    cp /home/pi/portsdown/images/testcards/75cbw.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "1080p" ]; then
    VIDEO_WIDTH=1920
    VIDEO_HEIGHT=1080

    cp /home/pi/portsdown/images/testcards/75cbw.jpg /home/pi/tmp/testcard.jpg
  fi

elif [ "$TESTCARD" == "greyscale" ]; then

 if [ "$FORMAT" == "4:3" ]; then
    VIDEO_WIDTH=720
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/11g.jpg /home/pi/tmp/testcard.jpg
 
  elif [ "$FORMAT" == "16:9" ]; then
    VIDEO_WIDTH=1024
    VIDEO_HEIGHT=576

    cp /home/pi/portsdown/images/testcards/11gw16.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "720p" ]; then
    VIDEO_WIDTH=1280
    VIDEO_HEIGHT=720

    cp /home/pi/portsdown/images/testcards/11gw.jpg /home/pi/tmp/testcard.jpg

  elif [ "$FORMAT" == "1080p" ]; then
    VIDEO_WIDTH=1920
    VIDEO_HEIGHT=1080

    cp /home/pi/portsdown/images/testcards/11gw.jpg /home/pi/tmp/testcard.jpg
  fi

fi


