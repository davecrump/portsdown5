#!/bin/bash

# Portsdown 5 Install by davecrump on 20240508


BuildLogMsg() {
  if [[ "$1" != "0" ]]; then
    echo $(date -u) "Build Fail    " "$2" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
    BUILD_STATUS="Fail"
  fi
}

BUILD_STATUS="Success"

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

if [ "$1" == "-d" ]; then
  GIT_SRC="davecrump";
  echo
  echo "--------------------------------------------------------"
  echo "----- Installing development version of Portsdown 5-----"
  echo "--------------------------------------------------------"
  echo $(date -u) "Installing Dev Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
elif [ "$1" == "-u" -a ! -z "$2" ]; then
  GIT_SRC="$2"
  echo
  echo "WARNING: Installing ${GIT_SRC} development version, press enter to continue or 'q' to quit."
  read -n1 -r -s key;
  if [[ $key == q ]]; then
    exit 1;
  fi
  echo "ok!";
  echo $(date -u) "Installing ${GIT_SRC} Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
else
  GIT_SRC="britishamateurtelevisionclub";
  echo
  echo "----------------------------------------------------------------"
  echo "----- Installing BATC Production version of Portsdown 4 NG -----"
  echo "----------------------------------------------------------------"
  echo $(date -u) "Installing Production Version" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
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

# Upgrade the distribution
echo
echo "-----------------------------------"
echo "----- Performing dist-upgrade -----"
echo "-----------------------------------"
sudo apt-get -y dist-upgrade
  SUCCESS=$?; BuildLogMsg $SUCCESS "dist-upgrade"

# Install the packages that we need
echo
echo "-------------------------------"
echo "----- Installing Packages -----"
echo "-------------------------------"

sudo apt-get -y install git                              # For app download
  SUCCESS=$?; BuildLogMsg $SUCCESS "git install"
sudo apt-get -y install cmake                            # For compiling
  SUCCESS=$?; BuildLogMsg $SUCCESS "cmake install"
sudo apt-get -y install fbi                              # For display of images
  SUCCESS=$?; BuildLogMsg $SUCCESS "fbi install"
sudo apt-get -y install libfftw3-dev                     # For bandviewers
  SUCCESS=$?; BuildLogMsg $SUCCESS "libfftw3-dev install"
sudo apt-get -y install nginx-light                      # For web access
  SUCCESS=$?; BuildLogMsg $SUCCESS "libfcgi-dev install"
sudo apt-get -y install libfcgi-dev                      # For web control
  SUCCESS=$?; BuildLogMsg $SUCCESS "nginx-light install"
sudo apt-get -y install libjpeg-dev                      #
  SUCCESS=$?; BuildLogMsg $SUCCESS "libjpeg-dev install"
sudo apt-get -y install imagemagick                      # for captions
  SUCCESS=$?; BuildLogMsg $SUCCESS "imagemagick install"
sudo apt-get -y install libraspberrypi-dev               # for bcm_host
  SUCCESS=$?; BuildLogMsg $SUCCESS "libraspberrypi-dev install"
sudo apt-get -y install libpng-dev                          # for screen capture
  SUCCESS=$?; BuildLogMsg $SUCCESS "libpng-dev install"
sudo apt-get -y install vlc                                 # for rx and replay
  SUCCESS=$?; BuildLogMsg $SUCCESS "vlc install"

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

# Install LimeSuite 23.11 as at 8 May 2024
# Commit 9dce3b6a6bd66537a2249ad27101345d31aafc89
#echo
#echo "--------------------------------------"
#echo "----- Installing LimeSuite 22.09 -----"
#echo "--------------------------------------"

#wget https://github.com/myriadrf/LimeSuite/archive/9dce3b6a6bd66537a2249ad27101345d31aafc89.zip -O master.zip
#  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite download"

# Copy into place
#unzip -o master.zip
#cp -f -r LimeSuite-9dce3b6a6bd66537a2249ad27101345d31aafc89 LimeSuite
#rm -rf LimeSuite-9dce3b6a6bd66537a2249ad27101345d31aafc89
#rm master.zip

# Compile LimeSuite
#cd LimeSuite/
#mkdir dirbuild
#cd dirbuild/
#cmake ../
#  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite cmake"
#make -j 4 -O
#  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite make"
#sudo make install
#sudo ldconfig
#cd /home/pi

# Install udev rules for LimeSuite
#cd LimeSuite/udev-rules
#chmod +x install.sh
#sudo /home/pi/LimeSuite/udev-rules/install.sh
#  SUCCESS=$?; BuildLogMsg $SUCCESS "LimeSuite udev rules"
#cd /home/pi	

# Record the LimeSuite Version	
#echo "9dce3b6" >/home/pi/LimeSuite/commit_tag.txt

# Download the previously selected version of Portsdown 5
echo
echo "--------------------------------------------"
echo "----- Downloading Portsdown 5 Software -----"
echo "--------------------------------------------"

wget https://github.com/${GIT_SRC}/portsdown5/archive/main.zip
  SUCCESS=$?; BuildLogMsg $SUCCESS "Portsdown 5 download from GitHub"

# Unzip the portsdown software and copy to the Pi
unzip -o main.zip
mv portsdown5-main portsdown
rm main.zip
cd /home/pi

# Compile the main Portsdown 5 application
echo
echo "---------------------------------"
echo "----- Compiling Portsdown 5 -----"
echo "---------------------------------"

cd /home/pi/portsdown/src/gui
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "Portsdown 5 compile"
mv /home/pi/portsdown/src/gui/portsdown5 /home/pi/portsdown/bin/portsdown5
cd /home/pi

# Compile Lime BandViewer
echo
echo "-------------------------------------"
echo "----- Compiling Lime BandViewer -----"
echo "-------------------------------------"

cd /home/pi/portsdown/src/limeview
make -j 4 -O
  SUCCESS=$?; BuildLogMsg $SUCCESS "Lime BandViewer compile"

mv /home/pi/portsdown/src/limeview/limeview /home/pi/portsdown/bin/limeview

# Copy the fftw wisdom file to home so that there is no start-up delay
# This file works for both BandViewer and NF Meter
#cp .fftwf_wisdom /home/pi/.fftwf_wisdom
cd /home/pi



# Rotate the touchscreen display to the normal orientation 
if !(grep video=DSI-1 /boot/firmware/cmdline.txt) then
  sudo sed -i '1s,$, video=DSI-1:800x480M@60\,rotate=180,' /boot/firmware/cmdline.txt
fi

# Set the default HDMI 0 output to 720p60
if !(grep video=HDMI-A-1 /boot/firmware/cmdline.txt) then
  sudo sed -i '1s,$, video=HDMI-A-1:1280x720M@60,' /boot/firmware/cmdline.txt
fi

# Disable the flash screen
if !(grep disable_splash /boot/firmware/config.txt) then
  sudo sh -c "echo disable_splash=1 >> /boot/firmware/config.txt"
fi

# Stop the cursor flashing on the touchscreen
if !(grep global_cursor_default /boot/firmware/cmdline.txt) then
  sudo sed -i '1s,$, vt.global_cursor_default=0,' /boot/firmware/cmdline.txt
fi

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

# Install the command-line aliases
echo "alias ugui='/home/pi/portsdown/utils/uguir.sh'"  >> /home/pi/.bash_aliases

# Record the Version Number
head -c 9 /home/pi/portsdown/version_history.txt > /home/pi/portsdown/configs/installed_version.txt
echo -e "\n" >> /home/pi/portsdown/configs/installed_version.txt
echo "Version number" | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null
head -c 9 /home/pi/portsdown/version_history.txt | sudo tee -a /home/pi/p5_initial_build_log.txt  > /dev/null

echo
echo "SD Card Serial:"
cat /sys/block/mmcblk0/device/cid


if [ "$BUILD_STATUS" == "Fail" ] ; then
  echo
  echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  echo "!!!!!  Build complete with errors   !!!!!!!!"
  echo "!!!!!  Read log below for details   !!!!!!!!"
  echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  echo
  cat /home/pi/p5_initial_build_log.txt
  echo
  echo enter sudo reboot now to manually reboot
  exit
else
  echo
  echo "------------------------------------------"
  echo "-----  Successful build, no errors   -----"
  echo "-----                                -----"
  echo "-----         Rebooting now          -----"
  echo "------------------------------------------"
  sudo reboot now
fi

exit





