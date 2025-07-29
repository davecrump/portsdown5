#!/bin/bash

# Portsdown 5 Install/Update script by davecrump

# Define Function to Read from Config File
get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

# Define function to write messages to build log
BuildLogMsg() {
  if [[ "$1" != "0" ]]; then
    echo $(date -u) "Build Fail    " "$2" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
    BUILD_STATUS="Fail"
  fi
}

# Define function to write messages to screen during update
DisplayUpdateMsg() {
  # Delete any old update message image
  rm /home/pi/tmp/update.jpg >/dev/null 2>/dev/null

  # Create the update image in the tempfs folder
  convert -font "FreeSans" -size 800x480 xc:white \
    -gravity North -pointsize 40 -annotate 0 "Updating Portsdown Software" \
    -gravity Center -pointsize 50 -annotate 0 "$1""\n\nPlease wait" \
    -gravity South -pointsize 50 -annotate 0 "DO NOT TURN POWER OFF" \
    /home/pi/tmp/update.jpg

  # Rotate for screen orientation
  convert /home/pi/tmp/update.jpg -rotate "$FBORIENTATION" /home/pi/tmp/update.jpg

  # Display the update message on the desktop
  sudo fbi -T 1 -noverbose /home/pi/tmp/update.jpg >/dev/null 2>/dev/null
  (sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work
  /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
}

################################## START HERE #####################################

BUILD_STATUS="Success"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"
FBORIENTATION="0"

# Create first entry for the build error log
echo $(date -u) "New Build started" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
sudo chown pi:pi /home/pi/p5_initial_build_log.txt

# Check current user
whoami | grep -q pi
if [ $? != 0 ]; then
  echo "Install must be performed as user pi"
  BuildLogMsg "1" "Exiting, not user pi"
  exit
fi

# Check for 64 bit bookworm
cat /etc/os-release | grep -q 'Debian GNU/Linux 12 (bookworm)'
if [ $? != 0 ]; then
  echo
  echo "Portsdown 5 requires the 64 bit version of bookworm lite"
  echo "Please rebuild your SD Card with the correct OS"
  echo 
  echo "Press any key to exit"
  read -n 1
  printf "\n"
  if [[ "$REPLY" = "d" || "$REPLY" = "D" ]]; then  # Allow to proceed for development
    echo "Continuing build......"
    BuildLogMsg "1" "Continuing build, but NOT bookworm 64 bit"
  else
    BuildLogMsg "1" "Exiting, not bookworm 64 bit"
    exit
  fi
fi

# Parse the arguments
GIT_SRC="BritishAmateurTelevisionClub";  # default
LMNDE="false"                            # don't load LimeSuiteNG and LimeNET Micro 2.0 DE specifics unless requested
WAIT="false"                             # Go straight into reboot unless requested
UPDATE="false"                           # set to true for an update
POSITIONAL_ARGS=()

while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--development)
      if [[ "$GIT_SRC" == "BritishAmateurTelevisionClub" ]]; then
        GIT_SRC="davecrump"
      fi
      shift # past argument
      ;;
    -w|--wait)
      WAIT="true"
      shift # past argument
      ;;
    -u|--update)
      UPDATE="true"
      shift # past argument
      ;;
    -x|--xtrx)
      LMNDE="true" 
      shift # past argument
      ;;
    -g|--github)
      shift # past argument
      echo user"$1"
      GIT_SRC=$1
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      echo "Valid options:"
      echo "-d or --development          Build Portsdown 5 from development repository"
      echo "-w or --wait                 Wait at the end of Stage 1 of the build before reboot"
      echo "-x or --xtrx                 Include the files and modules for the LimeNET Micro 2.0 DE"
      echo "-g or --github githubname    Build Portsdown 5 from githubname/portsdown5 repository"
      echo "-u or --update               Update existing install.  May be used with -d "
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done
set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters

# Act on arguments
if [ "$UPDATE" == "true" ]; then
  echo "-------------------------------------------------------"
  echo "----- Updating an existing Portsdown 5 build ----------"
  echo "-------------------------------------------------------"
  echo $(date -u) "Updating - NOT new install" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

  # Stop the existing Portsdown process
  /home/pi/portsdown/utils/stop.sh &

  # Check orienttation and display update message
  FBORIENTATION=$(get_config_var fborientation $SCONFIGFILE)
  DisplayUpdateMsg "Starting Software Update"
fi

if [ "$GIT_SRC" == "davecrump" ]; then
  echo "--------------------------------------------------------"
  echo "----- Installing development version of Portsdown 5-----"
  echo "--------------------------------------------------------"
  echo $(date -u) "Installing Dev Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
elif [ "$GIT_SRC" == "BritishAmateurTelevisionClub" ]; then
  echo "-------------------------------------------------------------"
  echo "----- Installing BATC Production version of Portsdown 5 -----"
  echo "-------------------------------------------------------------"
  echo $(date -u) "Installing Production Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
else
  echo "WARNING: Installing ${GIT_SRC} development version, press enter to continue or 'q' to quit."
  read -n1 -r -s key;
  if [[ $key == q ]]; then
    exit 1;
  fi
  echo "ok!"
  echo $(date -u) "Installing ${GIT_SRC} Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
fi
if [ "$LMNDE" == "true" ]; then
  echo
  echo Installing extras for LimeNET Micro DE 2.0
  echo $(date -u) "Installing LimeNET Micro DE 2.0" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
fi
if [ "$WAIT" == "true" ]; then
  echo
  echo Manual reboot before Install Stage 2 requested.
  echo $(date -u) "Manual reboot before Install Stage 2 requested" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
fi


# Update the package manager
echo
echo "------------------------------------"
echo "----- Updating Package Manager -----"
echo "------------------------------------"
sudo dpkg --configure -a
  SUCCESS=$?; BuildLogMsg $SUCCESS "dpkg configure"
sudo apt-get update --allow-releaseinfo-change
  SUCCESS=$?; BuildLogMsg $SUCCESS "apt-get update"

# Uninstall the apt-listchanges package to allow silent install of ca certificates (201704030)
# http://unix.stackexchange.com/questions/124468/how-do-i-resolve-an-apparent-hanging-update-process
sudo apt-get -y remove apt-listchanges
  SUCCESS=$?; BuildLogMsg $SUCCESS "remove apt-listchanges"

# Exit here if /var/lib/dpkg/lock-frontend is not available yet
if [[ "$SUCCESS" != "0" ]]; then
  echo
  echo "Early error (probably a locked file).  Nothing changed on card, so please try again"
  exit
fi

# Upgrade the distribution
echo
echo "-----------------------------------"
echo "----- Performing dist-upgrade -----"
echo "-----------------------------------"
sudo apt-get -y dist-upgrade
  SUCCESS=$?; BuildLogMsg $SUCCESS "dist-upgrade"

if [ "$UPDATE" == "false" ]; then

  # Install the packages that we need
  echo
  echo "-------------------------------"
  echo "----- Installing Packages -----"
  echo "-------------------------------"

  sudo apt-get -y install git                                     # For app download
    SUCCESS=$?; BuildLogMsg $SUCCESS "git install"
  sudo apt-get -y install cmake                                   # For compiling
    SUCCESS=$?; BuildLogMsg $SUCCESS "cmake install"
  sudo apt-get -y install fbi                                     # For display of images
    SUCCESS=$?; BuildLogMsg $SUCCESS "fbi install"
  sudo apt-get -y install libusb-1.0-0-dev                        # For LimeSuite
    SUCCESS=$?; BuildLogMsg $SUCCESS "libusb-1.0-0-dev"
  sudo apt-get -y install linux-headers-generic                   # For LimeSuite
    SUCCESS=$?; BuildLogMsg $SUCCESS "linux-headers-generic install"
  sudo apt-get -y install libfftw3-dev                            # For bandviewers
    SUCCESS=$?; BuildLogMsg $SUCCESS "libfftw3-dev install"
  sudo apt-get -y install nginx-light                             # For web access
    SUCCESS=$?; BuildLogMsg $SUCCESS "libfcgi-dev install"
  sudo apt-get -y install libfcgi-dev                             # For web control
    SUCCESS=$?; BuildLogMsg $SUCCESS "nginx-light install"
  sudo apt-get -y install libjpeg-dev                             #
    SUCCESS=$?; BuildLogMsg $SUCCESS "libjpeg-dev install"
  sudo apt-get -y install imagemagick                             # for captions
    SUCCESS=$?; BuildLogMsg $SUCCESS "imagemagick install"
  sudo apt-get -y install libraspberrypi-dev                      # for bcm_host
    SUCCESS=$?; BuildLogMsg $SUCCESS "libraspberrypi-dev install"
  sudo apt-get -y install libpng-dev                              # for screen capture
    SUCCESS=$?; BuildLogMsg $SUCCESS "libpng-dev install"
  sudo apt-get -y install vlc                                     # for rx and replay
    SUCCESS=$?; BuildLogMsg $SUCCESS "vlc install"
  sudo apt-get -y install ffmpeg                                  # for tx encoding
    SUCCESS=$?; BuildLogMsg $SUCCESS "ffmpeg install"
  sudo apt-get -y install netcat-openbsd                          # For TS input
    SUCCESS=$?; BuildLogMsg $SUCCESS "netcat install"
  sudo apt-get -y install evemu-tools                             # For mouse device evaluation
    SUCCESS=$?; BuildLogMsg $SUCCESS "evemu-tools install"
  sudo apt-get -y install mosquitto                               # For F5OEO firmware on Pluto and LibreSDR
    SUCCESS=$?; BuildLogMsg $SUCCESS "mosquitto install"
  sudo apt-get -y install mosquitto-clients                       # For F5OEO firmware on Pluto and LibreSDR
    SUCCESS=$?; BuildLogMsg $SUCCESS "mosquitto-clients install"
  #sudo apt-get -y install libgmock-dev                           # For LimeSuiteNG 
    #SUCCESS=$?; BuildLogMsg $SUCCESS "libgmock-dev install"
  sudo apt-get -y install libasound2-dev                          # For LongMynd
    SUCCESS=$?; BuildLogMsg $SUCCESS "libasound2-dev install"
  sudo apt-get -y install libavahi-client-dev                     # For LibreSDR??
    SUCCESS=$?; BuildLogMsg $SUCCESS "libavahi-client-dev install"
  sudo apt-get -y install libzstd-dev                             # For libiio
    SUCCESS=$?; BuildLogMsg $SUCCESS "libzstd-dev"
  sudo apt-get -y install libxml2-dev                             # For libiio
    SUCCESS=$?; BuildLogMsg $SUCCESS "libxml2-dev"
  sudo apt-get -y install bison                                   # For libiio
    SUCCESS=$?; BuildLogMsg $SUCCESS "bison"
  sudo apt-get -y install flex                                    # For libiio
    SUCCESS=$?; BuildLogMsg $SUCCESS "flex"
  sudo apt-get -y install libaio-dev                              # For libiio
    SUCCESS=$?; BuildLogMsg $SUCCESS "libaio-dev"
  sudo apt-get -y install pip                                     # For Langstone 3
    SUCCESS=$?; BuildLogMsg $SUCCESS "pip"
  sudo apt-get -y install gnuradio                                # For Langstone 3
    SUCCESS=$?; BuildLogMsg $SUCCESS "gnuradio"
  sudo apt-get -y install hackrf                                  # For Langstone 3
    SUCCESS=$?; BuildLogMsg $SUCCESS "hackrf"
  sudo apt-get -y install sshpass                                 # For Langstone 3
    SUCCESS=$?; BuildLogMsg $SUCCESS "sshpass"
  sudo apt-get -y install libairspy-dev                           # For Airspy Bandviewer
    SUCCESS=$?; BuildLogMsg $SUCCESS "libairspy-dev"
  sudo apt-get -y install expect                                  # For unattended installs
    SUCCESS=$?; BuildLogMsg $SUCCESS "expect"
  sudo apt-get -y install uhubctl                                 # For USB resets
    SUCCESS=$?; BuildLogMsg $SUCCESS "uhubctl"
  sudo apt-get -y install arp-scan                                # For List Network Devices
    SUCCESS=$?; BuildLogMsg $SUCCESS "arp-scan"
  sudo apt-get -y install mplayer                                 # For video monitor
    SUCCESS=$?; BuildLogMsg $SUCCESS "mplayer"
fi

# Placeholder for New packages during update

if [ "$UPDATE" == "false" ]; then

  # Amend /boot/firmware/cmdline.txt and /boot/firmware/config.txt
  echo
  echo "-----------------------------------------------"
  echo "----- Amending cmdline.txt and config.txt -----"
  echo "-----------------------------------------------"

  # Set touchscreen to 16 bit per pixel
  #sudo sed -i 's/800x480M@60/800x480M-16@60/' /boot/firmware/cmdline.txt

  # Rotate the touchscreen display to the normal orientation 
  if !(grep video=DSI-1 /boot/firmware/cmdline.txt) then
    sudo sed -i '1s,$, video=DSI-1:800x480M-16@60\,rotate=180,' /boot/firmware/cmdline.txt
  fi

  # Rotate the touchscreen 2 display to the normal orientation 
  if !(grep video=DSI-2 /boot/firmware/cmdline.txt) then
    sudo sed -i '1s,$, video=DSI-2:720x1280@60\,rotate=270,' /boot/firmware/cmdline.txt
  fi

  # Set the default HDMI 0 output to 720p60
  if !(grep video=HDMI-A-1 /boot/firmware/cmdline.txt) then
    sudo sed -i '1s,$, video=HDMI-A-1:1280x720M@60,' /boot/firmware/cmdline.txt
  fi

  # Disable the splash screen
  if !(grep -E '^disable_splash' /boot/firmware/config.txt) then
    sudo sh -c "echo disable_splash=1 >> /boot/firmware/config.txt"
  fi

  # Disable the automatic update of IP address on login screen (which overwrites menu)
  sudo mv /etc/issue.d/IP.issue /etc/issue.d/IP.issue.old

  # If required, add the specific parameters for LimeSDR XTRX and LMN 2.0 DE
  if [ "$LMNDE" == "true" ]; then
    sudo sh -c "echo dtoverlay=pcie-32bit-dma >> /boot/firmware/config.txt"
    sudo sh -c "echo dtparam=spi=on >> /boot/firmware/config.txt"
    sudo sh -c "echo dtoverlay=spi1-2cs,cs0_pin=18,cs1_pin=17 >> /boot/firmware/config.txt"
    sudo sh -c "echo dtparam=i2c_vc=on >> /boot/firmware/config.txt"
    sudo sh -c "echo dtoverlay=i2c-rtc,pcf85063a,i2c_csi_dsi >> /boot/firmware/config.txt"
  fi

  # Stop the cursor flashing on the touchscreen
  if !(grep global_cursor_default /boot/firmware/cmdline.txt) then
    sudo sed -i '1s,$, vt.global_cursor_default=0,' /boot/firmware/cmdline.txt
  fi

  # Install the command-line aliases
  echo "alias ugui='/home/pi/portsdown/utils/uguir.sh'" >> /home/pi/.bash_aliases
  echo "alias gui='/home/pi/portsdown/utils/guir.sh'"  >> /home/pi/.bash_aliases
  echo "alias stop='/home/pi/portsdown/utils/stop.sh'"  >> /home/pi/.bash_aliases

  echo
  echo "Amendments made"
fi

if [ "$UPDATE" == "false" ]; then
  echo
  echo "-------------------------------"
  echo "----- Installing WiringPi -----"
  echo "-------------------------------"

  cd /home/pi
  git clone https://github.com/WiringPi/WiringPi.git
    SUCCESS=$?; BuildLogMsg $SUCCESS "Wiring Pi Git Clone"
  cd WiringPi
  ./build debian
    SUCCESS=$?; BuildLogMsg $SUCCESS "Wiring Pi Build"

  ## Read latest version number
  vMaj=`cut -d. -f1 VERSION`
  vMin=`cut -d. -f2 VERSION`

  mv debian-template/wiringpi_"$vMaj"."$vMin"_arm64.deb .
    SUCCESS=$?; BuildLogMsg $SUCCESS "Moved wiringpi_3.16_arm64.deb"
  sudo apt install ./wiringpi_"$vMaj"."$vMin"_arm64.deb
    SUCCESS=$?; BuildLogMsg $SUCCESS "Installed Wiring Pi"
  cd /home/pi

fi

if [ "$UPDATE" == "true" ]; then

  echo
  echo "-------------------------------"
  echo "----- Preserving Configs ------"
  echo "-------------------------------"

  rm -rf /home/pi/configs
  mkdir /home/pi/configs/
  cp -r /home/pi/portsdown/configs/*.txt /home/pi/configs/

  # Delete previous Portsdown files
  rm -rf /home/pi/portsdown

  DisplayUpdateMsg "Downloading new Software"
fi

# Download the previously selected version of Portsdown 5
echo
echo "--------------------------------------------"
echo "----- Downloading Portsdown 5 Software -----"
echo "--------------------------------------------"

cd /home/pi
wget https://github.com/${GIT_SRC}/portsdown5/archive/main.zip -O main.zip
  SUCCESS=$?; BuildLogMsg $SUCCESS "Portsdown 5 download from GitHub"
# Unzip the portsdown software and copy to the Pi
unzip -o main.zip
mv portsdown5-main portsdown
rm main.zip
mkdir /home/pi/portsdown/bin

if [ "$UPDATE" == "true" ]; then

  echo
  echo "------------------------------"
  echo "----- Restoring Configs ------"
  echo "------------------------------"

  # Copy configs
  cp -rf /home/pi/configs/*.txt /home/pi/portsdown/configs/

  # Remove the old directory
  rm -rf /home/pi/configs/
  DisplayUpdateMsg "Compiling new Software"
fi

# Compile the main Portsdown 5 application
echo
echo "---------------------------------"
echo "----- Compiling Portsdown 5 -----"
echo "---------------------------------"

cd /home/pi/portsdown/src/portsdown
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "Portsdown 5 compile"
mv /home/pi/portsdown/src/portsdown/portsdown5 /home/pi/portsdown/bin/portsdown5
cd /home/pi

if [ "$UPDATE" == "false" ]; then
  # Install LimeSuite 23.11 as at 8 May 2024
  # Commit 9dce3b6a6bd66537a2249ad27101345d31aafc89
  echo
  echo "--------------------------------------"
  echo "----- Installing LimeSuite 22.09 -----"
  echo "--------------------------------------"

  wget https://github.com/myriadrf/LimeSuite/archive/9dce3b6a6bd66537a2249ad27101345d31aafc89.zip -O master.zip
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite download"

  # Copy into place
  unzip -o master.zip
  cp -f -r LimeSuite-9dce3b6a6bd66537a2249ad27101345d31aafc89 LimeSuite
  rm -rf LimeSuite-9dce3b6a6bd66537a2249ad27101345d31aafc89
  rm master.zip

  # Compile LimeSuite
  cd LimeSuite/
  mkdir dirbuild
  cd dirbuild/
  cmake ../
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite cmake"
  make -j 4 -O
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite make"
  sudo make install
  sudo ldconfig
  cd /home/pi

  # Install udev rules for LimeSuite
  cd LimeSuite/udev-rules
  chmod +x install.sh
  sudo /home/pi/LimeSuite/udev-rules/install.sh
    SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite udev rules"
  cd /home/pi	

  # Record the LimeSuite Version	
  echo "9dce3b6" >/home/pi/LimeSuite/commit_tag.txt

  # Install libiio for Pluto
  echo
  echo "---------------------------------------"
  echo "----- Installing Libiio for Pluto -----"
  echo "---------------------------------------"

  cd /home/pi
  git clone https://github.com/analogdevicesinc/libiio.git
    SUCCESS=$?; BuildLogMsg $SUCCESS "git clone libiio"
  cd libiio
  cmake ./
    SUCCESS=$?; BuildLogMsg $SUCCESS "cmake libiio"
  make all
    SUCCESS=$?; BuildLogMsg $SUCCESS "make libiio"
  sudo make install
  cd /home/pi

  echo
  echo "------------------------------"
  echo "----- Installing RTL-SDR -----"
  echo "------------------------------"

  cd /home/pi/portsdown/src
  git clone git://git.osmocom.org/rtl-sdr.git
    SUCCESS=$?; BuildLogMsg $SUCCESS "git clone rtl-sdr"
  cd rtl-sdr
  mkdir build
  cd build
  cmake ../ -DINSTALL_UDEV_RULES=ON
    SUCCESS=$?; BuildLogMsg $SUCCESS "cmake rtl-sdr"
  make
    SUCCESS=$?; BuildLogMsg $SUCCESS "make rtl-sdr"
  sudo make install
  sudo cp ../rtl-sdr.rules /etc/udev/rules.d/
  sudo ldconfig

  # Create a new /etc/modprobe.d/blacklist-rtl.conf file
  sudo cat > blacklist-rtl.conf << EOF
blacklist dvb_usb_rtl28xxu
blacklist rtl2832
blacklist rtl2830
EOF
  sudo mv blacklist-rtl.conf /etc/modprobe.d/blacklist-rtl.conf

  echo
  echo "------------------------------------------"
  echo "----- Installing DATV Express Server -----"
  echo "------------------------------------------"

  cd /home/pi
  wget https://github.com/G4GUO/express_server/archive/master.zip
    SUCCESS=$?; BuildLogMsg $SUCCESS "express_server download"
  unzip master.zip
  mv express_server-master /home/pi/portsdown/src/express_server
  rm master.zip
  cd /home/pi/portsdown/src/express_server
  make
    SUCCESS=$?; BuildLogMsg $SUCCESS "make express_server"
  sudo make install

  echo
  echo "----------------------------------------"
  echo "----- Preparing to install SDRPlay -----"
  echo "----------------------------------------"

  # Download api
  cd /home/pi
  wget https://www.sdrplay.com/software/SDRplay_RSP_API-Linux-3.15.2.run
  chmod +x SDRplay_RSP_API-Linux-3.15.2.run

  # Create file to trigger install on next reboot
  touch /home/pi/portsdown/.post-install_actions
  cd /home/pi


fi

# Compile legacy LimeSDR Toolbox
echo
echo "--------------------------------------------"
echo "----- Compiling Legacy LimeSDR Toolbox -----"
echo "--------------------------------------------"

# Compile dvb2iq first
cd /home/pi/portsdown/src/limesdr_toolbox/libdvbmod/libdvbmod
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "libdvbmod compile"
cd /home/pi/portsdown/src/limesdr_toolbox/libdvbmod/DvbTsToIQ
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "DvbTsToIQ compile"
mv dvb2iq /home/pi/portsdown/bin/

# and then LimeSDR toolbox
cd /home/pi/portsdown/src/limesdr_toolbox/
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSDR Toolbox compile"
make dvb
  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSDR DVB compile"

mv limesdr_send /home/pi/portsdown/bin/
mv limesdr_dump /home/pi/portsdown/bin/
mv limesdr_stopchannel /home/pi/portsdown/bin/
mv limesdr_forward /home/pi/portsdown/bin/
mv limesdr_dvb /home/pi/portsdown/bin/

# Compile Framebuffer capture utility
echo
echo "-------------------------------------------------"
echo "----- Compiling Framebuffer Capture Utility -----"
echo "-------------------------------------------------"

cd /home/pi/portsdown/src/fb2png
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "fb2png compile"

mv /home/pi/portsdown/src/fb2png/fb2png /home/pi/portsdown/bin/fb2png

# Compile LongMynd
echo
echo "-------------------------------------------"
echo "----- Compiling the LongMynd Receiver -----"
echo "-------------------------------------------"
cd /home/pi
cp -r /home/pi/portsdown/src/longmynd/ /home/pi/
cd longmynd
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "LongMynd compile"

# Set up the udev rules for USB
sudo cp minitiouner.rules /etc/udev/rules.d/

# Compile Legacy Lime BandViewer
echo
echo "--------------------------------------------"
echo "----- Compiling Legacy Lime BandViewer -----"
echo "--------------------------------------------"

cd /home/pi/portsdown/src/limeview
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "Lime BandViewer compile"

mv /home/pi/portsdown/src/limeview/limeview /home/pi/portsdown/bin/limeview

# Copy the fftw wisdom file to home so that there is no start-up delay
# This file works for both BandViewer and NF Meter
#cp .fftwf_wisdom /home/pi/.fftwf_wisdom
cd /home/pi

# Compile Legacy Noise Figure Meter
#echo
#echo "-----------------------------------------------"
#echo "----- Compiling Legacy Noise Figure Meter -----"
#echo "-----------------------------------------------"

#cd /home/pi/portsdown/src/nf_meter
#make -j 4 -O
#  SUCCESS=$?; BuildLogMsg $SUCCESS "Lime NF Meter compile"

#mv /home/pi/portsdown/src/nf_meter/nf_meter /home/pi/portsdown/bin/nf_meter
#cd /home/pi

# Compile Legacy Noise Meter
#echo
#echo "----------------------------------------"
#echo "----- Compiling Legacy Noise Meter -----"
#echo "----------------------------------------"

#cd /home/pi/portsdown/src/noise_meter
#make -j 4 -O
#  SUCCESS=$?; BuildLogMsg $SUCCESS "Lime Noise Meter compile"

#mv /home/pi/portsdown/src/noise_meter/noise_meter /home/pi/portsdown/bin/noise_meter
#cd /home/pi

if [ "$UPDATE" == "false" ]; then

  # Download, compile and install DATV Express-server
  echo
  echo "------------------------------------------"
  echo "----- Installing DATV Express Server -----"
  echo "------------------------------------------"
  cd /home/pi
  wget https://github.com/G4GUO/express_server/archive/master.zip
    SUCCESS=$?; BuildLogMsg $SUCCESS "DATV Express Server Download"
  unzip master.zip
  mv express_server-master express_server
  rm master.zip
  cd /home/pi/express_server
  make
    SUCCESS=$?; BuildLogMsg $SUCCESS "DATV Express Server compile"
  sudo make install
  cd /home/pi
fi

if [ "$UPDATE" == "false" ]; then

  # Configure to start Portsdown on boot as a service
  sudo cp /home/pi/portsdown/scripts/set-up_configs/portsdown.service /etc/systemd/system/
  sudo systemctl daemon-reload
  sudo systemctl enable portsdown

  # Configure the nginx web server
  cp -r /home/pi/portsdown/scripts/set-up_configs/webroot /home/pi/webroot
  sudo cp /home/pi/portsdown/scripts/set-up_configs/nginx.conf /etc/nginx/nginx.conf

  echo
  echo "------------------------------------------"
  echo "----- Setting up for captured images -----"
  echo "------------------------------------------"

  # Amend /etc/fstab to create a tmpfs drive at ~/tmp for multiple images
  sudo sed -i '4itmpfs           /home/pi/tmp    tmpfs   defaults,noatime,nosuid,size=10m  0  0' /etc/fstab

  # Create a ~/snaps folder for captured images
  mkdir /home/pi/snaps

  # Set the image index number to 0
  echo "0" > /home/pi/snaps/snap_index.txt

  echo
  echo "SD Card Serial:"
  cat /sys/block/mmcblk0/device/cid

  # Create file to trigger install on next reboot
  touch /home/pi/portsdown/.post-install_actions
  cd /home/pi

  if [ "$BUILD_STATUS" == "Fail" ] ; then
    echo $(date -u) " Build stage 1 fail" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
    echo
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "!!!!!   Build stage 1 complete with errors   !!!!!!!"
    echo "!!!!!       Read log below for details       !!!!!!!"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo
    cat /home/pi/p5_initial_build_log.txt
    echo
    echo Enter sudo reboot now to manually reboot and run stage 2 anyway
    echo 
    exit
  else
    echo
    echo "--------------------------------------------"
    echo "-----  Successful stage 1, no errors   -----"
    echo "-----                                  -----"
    echo "-----           Rebooting now          -----"
    echo "--------------------------------------------"

    if [ "$WAIT" == "true" ]; then                 ## Wait for key press before reboot
      echo "-----                                  -----"
      echo "-----       Waiting for Reboot         -----"
      echo "--------------------------------------------"
      echo
      read -p "Press enter to reboot for stage 2 of the installation"
    fi

    echo "-----                                  -----"
    echo "-----           Rebooting now          -----"
    echo "--------------------------------------------"

    sudo reboot now
  fi

else   # Update

  DisplayUpdateMsg "Update Complete.  Rebooting"
  echo $(date -u) " Update Complete" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

  # Record the Version Number
  head -c 9 /home/pi/portsdown/version_history.txt > /home/pi/portsdown/configs/installed_version.txt
  echo -e "\n" >> /home/pi/portsdown/configs/installed_version.txt
  echo "Version number" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
  head -c 9 /home/pi/portsdown/version_history.txt | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

  echo
  echo "--------------------------------------------"
  echo "-----        Update Complete           -----"
  echo "-----                                  -----"
  echo "-----           Rebooting now          -----"
  echo "--------------------------------------------"

  sleep 1
  sudo reboot now

fi

exit





