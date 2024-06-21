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
#  /home/pi/rpidatv/scripts/single_screen_grab_for_web.sh &
}

BUILD_STATUS="Success"

# Check if LimeSuiteNG needs to be installed (requires reboot after config.txt amendments in Stage 1)
if (grep 'Installing LimeNET Micro DE' /home/pi/p5_initial_build_log.txt) then

  DisplayUpdateMsg "Build stage 2, step 1\nInstalling LimeSuiteNG"

  cd /home/pi
  echo
  echo "----------------------------------------"
  echo "-----    Installing LimeSuiteNG    -----"
  echo "----------------------------------------"

  git clone https://github.com/myriadrf/LimeSuiteNG        # Download LimeSuiteNG
    SUCCESS=$?; BuildLogMsg $SUCCESS "git clone LimeSuiteNG"

  # Dependencies from install_dependencies.sh
  # build-essential and cmake already installed

  sudo apt-get -y install --no-install-recommends linux-headers-generic
  SUCCESS=$?; BuildLogMsg $SUCCESS "linux-headers-generic install"

  sudo apt-get -y install --no-install-recommends libsoapysdr-dev
    SUCCESS=$?; BuildLogMsg $SUCCESS "libsoapysdr-dev install"

  sudo apt-get -y install --no-install-recommends libusb-1.0-0-dev
    SUCCESS=$?; BuildLogMsg $SUCCESS "libusb-1.0-0-dev install"

  sudo apt-get -y install --no-install-recommends libwxgtk3.2-dev
    SUCCESS=$?; BuildLogMsg $SUCCESS "libwxgtk3.2-dev install"

  cd LimeSuiteNG

  cmake -B build
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG cmake"

  cd build

  make -j 4 -O
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG make"

  sudo make install
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG make install"

  sudo ldconfig
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuiteNG ldconfig"

  cd /home/pi

  # Compile Legacy Lime BandViewer
  echo
  echo "----------------------------------------"
  echo "----- Compiling NG Lime BandViewer -----"
  echo "----------------------------------------"

  cd /home/pi/portsdown/src/limeviewng
  make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "Lime BandViewer NG compile"

  mv /home/pi/portsdown/src/limeviewng/limeviewng /home/pi/portsdown/bin/limeviewng

fi

# Record the Version Number
head -c 9 /home/pi/portsdown/version_history.txt > /home/pi/portsdown/configs/installed_version.txt
echo -e "\n" >> /home/pi/portsdown/configs/installed_version.txt
echo "Version number" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
head -c 9 /home/pi/portsdown/version_history.txt | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

if [ "$BUILD_STATUS" == "Fail" ] ; then
  DisplayUpdateMsg "Build stage 2, failed\nread the log at\n/home/pi/p5_initial_build_log.txt"
  sleep 10
fi

cd /home/pi

# Delete the trigger file
rm /home/pi/portsdown/.post-install_actions

exit