#!/bin/bash

# This file is run (from startup.sh) on the first reboot after an install (or update if required).

BuildLogMsg() {
  if [[ "$1" != "0" ]]; then
    echo $(date -u) "Build Fail    " "$2" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
    BUILD_STATUS="Fail"
  fi
}

DisplayUpdateMsg() {
  # Delete any old update message image
  rm /home/pi/tmp/update.jpg >/dev/null 2>/dev/null

  # Create the update image in the tempfs folder
  convert -font "FreeSans" -size 800x480 xc:white \
    -gravity North -pointsize 40 -annotate 0,0,0,20 "Installing Portsdown Software" \
    -gravity Center -pointsize 50 -annotate 0 "$1""\n\nPlease wait" \
    -gravity South -pointsize 50 -annotate 0,0,0,20 "DO NOT TURN POWER OFF" \
    /home/pi/tmp/update.jpg

  # Display the update message on the desktop
  sudo fbi -T 1 -noverbose -a /home/pi/tmp/update.jpg >/dev/null 2>/dev/null
  (sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work
  /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
}

echo $(date -u) "Build Stage 2 Started" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

BUILD_STATUS="Success"

# Check if LimeSuiteNG needs to be installed (requires reboot after config.txt amendments in Stage 1)
if (grep 'Installing LimeNET Micro DE' /home/pi/p5_initial_build_log.txt) then

  DisplayUpdateMsg "Build stage 2, step 1\nInstalling LimeSuiteNG"

  cd /home/pi
  echo
  echo "----------------------------------------"
  echo "-----    Installing LimeSuiteNG    -----"
  echo "----------------------------------------"

#  git clone https://github.com/myriadrf/LimeSuiteNG        # Download LimeSuiteNG
#    SUCCESS=$?; BuildLogMsg $SUCCESS "git clone LimeSuiteNG"

#  cd LimeSuiteNG

#  cmake -B build
#    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG cmake"

#  cd build

#  make -j 4 -O
#    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG make"

#  sudo make install
#    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG make install"

#  sudo ldconfig
#    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG ldconfig"

  cd /home/pi

  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/pi/LimeSuiteNG/build/lib/

  # Compile NG Lime BandViewer
  echo
  echo "----------------------------------------"
  echo "----- Compiling NG Lime BandViewer -----"
  echo "----------------------------------------"

  #cd /home/pi/portsdown/src/limeviewng
  #make -j 4 -O
  #SUCCESS=$?; BuildLogMsg $SUCCESS "Lime BandViewer NG compile"

  #mv /home/pi/portsdown/src/limeviewng/limeviewng /home/pi/portsdown/bin/limeviewng

fi

# Install SDRPlay driver

cd /home/pi
if [ ! -f  SDRplay_RSP_API-Linux-3.15.2.run ]; then
  wget https://www.sdrplay.com/software/SDRplay_RSP_API-Linux-3.15.2.run
fi
chmod +x SDRplay_RSP_API-Linux-3.15.2.run

/home/pi/portsdown/scripts/set-up_configs/sdrplay_api_install.exp
  SUCCESS=$?; BuildLogMsg $SUCCESS "sdrplay api install"

cd /home/pi
rm SDRplay_RSP_API-Linux-3.15.2.run
sudo systemctl disable sdrplay  # service is started only when required


# Record the Version Number in the build log
printf "Version number: " | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
head -c 9 /home/pi/portsdown/version_history.txt | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
echo | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

if [ "$BUILD_STATUS" == "Fail" ] ; then
  echo $(date -u) "Build Stage 2 Fail" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
  DisplayUpdateMsg "Build stage 2, failed\nread the log at\n/home/pi/p5_initial_build_log.txt"
  sleep 10
else
  echo $(date -u) "Build Stage 2 Complete" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
fi

cd /home/pi

# Delete the trigger file
rm /home/pi/portsdown/.post-install_actions

exit