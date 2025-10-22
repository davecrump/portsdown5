#!/usr/bin/env bash

# set -x

# This script is sourced (run) by startup.sh if the touchscreen interface
# is required.
# It enables the various touchscreen applications to call each other
# by checking their return code
# If any applications exits abnormally (with a 1 or a 0 exit code)
# it currently terminates or (for interactive sessions) goes back to a prompt

############ Set Environment Variables ###############

PCONFIGFILE="/home/pi/portsdown/configs/portsdown_config.txt"
SCONFIGFILE="/home/pi/portsdown/configs/system_config.txt"

############ Function to Read from Config File ###############

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

########### Function to display message ####################

DisplayMsg() {
  # Delete any old update message image
  rm /home/pi/tmp/update.jpg >/dev/null 2>/dev/null

  # Create the update image in the tempfs folder
  convert -size 800x480 xc:black -font FreeSans -fill white \
    -gravity Center -pointsize 30 -annotate 0 "$1" \
    /home/pi/tmp/update.jpg

  # Display the update message on the desktop
#  sudo fbi -T 1 -noverbose -a /home/pi/tmp/update.jpg >/dev/null 2>/dev/null
#  (sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work
}

############ Function to check which BandViewer to start ####################

ChooseBandViewerSDR()
{
  # Set default start code to be Portsdown main menu
  BANDVIEW_START_CODE=129

  # Check for presence of Airspy   
  lsusb | grep -q 'Airspy'
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=140
    return
  fi

  # Check for presence of SDRplay   
  lsusb | grep -q '1df7:'
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=144
    return
  fi

  # Check for presence of Lime Mini
  lsusb | grep -q '0403:601f'
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=136
    return
  fi

  # Check for presence of Lime USB
  lsusb | grep -q '1d50:6108' # Lime USB
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=136
    return
  fi

  # Check for the presence of an RTL-SDR
  lsusb | grep -E -q "RTL|DVB"
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=141
    return
  fi 

  # Check for the presence of a Pluto (can false positive on 192.168.2.* LANs)
  # Look up Pluto IP
  PLUTOIP=$(get_config_var plutoip $PCONFIGFILE)
  sleep 10 # Give the Pluto time to boot!
  # Ping Pluto
  timeout 0.2 ping "$PLUTOIP" -c1 | head -n 5 | tail -n 1 | grep -o "1 received,"
  if [ $? == 0 ]; then   ## Present
    BANDVIEW_START_CODE=143
    return
  fi
}

##################################################################

# set -x

# Return Codes
#
# <128  Exit leaving system running
# 128  Exit leaving system running
# 129  Exit from any app requesting restart of main portsdown gui
# 130  Exit from portsdown gui requesting start of siggen
# 132  Run Update Script for production load
# 133  Run Update Script for development load
# 134  Exit from portsdown gui requesting start of the XY Display
# 135  Run the Langstone TRX V1
# 136  Exit from gui requesting start of LimeSDR BandViewer
# 137  Exit from portsdown gui requesting start of Power Meter
# 138  Exit from portsdown gui requesting start of NF Meter
# 139  Exit from portsdown gui requesting start of Sweeper
# 140  Exit from portsdown gui requesting start of Airspy BandViewer
# 141  Exit from portsdown gui requesting start of RTL-SDR BandViewer
# 142  Exit from portsdown gui requesting start of DMM Display
# 143  Exit from portsdown gui requesting start of Pluto BandViewer
# 144  Exit from portsdown gui requesting start of SDRPlay BandViewer
# 145  Run the Langstone TRX V2 Lime
# 146  Run the Langstone TRX V2 Pluto
# 147  Exit from portsdown gui requesting start of Noise Meter
# 148  Exit from portsdown gui requesting start of Pluto NF Meter
# 149  Exit from portsdown gui requesting start of Pluto Noise Meter
# 150  Run the Meteor Viewer
# 151  Exit from gui requesting start of LimeSDR BandViewer for LimeSuiteNG
# 152  Exit from gui requesting start of LimeSDR NF Meter for LimeSuiteNG
# 155  Exit from gui requesting start of preferred BandViewer
# 160  Shutdown from GUI
# 170  Spectrum Analyser Scheduler
# 171  Spectrum Analyser DATV RX on IF
# 172  Spectrum Analyser Direct DATV RX
# 173  Spectrum Analyser AD8318 power detector sweeper
# 174  Spectrum Analyser Bolo power Meter
# 175  Spectrum Analyser LimeSDR
# 176  Spectrum Analyser LimeSDR fft on IF
# 177  Spectrum Analyser Swept
# 178  Spectrum Analyser TG Swept
# 179  Spectrum Analyser TG Zero
# 181  Pico Viewer
# 192  Reboot from GUI
# 193  Rotate 7 inch and reboot
# 197  Start Ryde RX
# 198  Boot to Portsdown RX
# 199  Boot to Portsdown TX
# 207  Exit from any app requesting restart of main portsdown gui on Menu 7 (test equipment)

MODE_STARTUP=$(get_config_var startup $SCONFIGFILE)

# Display the web not enabled caption
# It will get over-written if web is enabled
#cp /home/pi/portsdown/scripts/images/web_not_enabled.png /home/pi/tmp/screen.png

case "$MODE_STARTUP" in
  Portsdown_boot)
    # Start the Portsdown Touchscreen
    GUI_RETURN_CODE=129
  ;;
  Bandview_boot)
    # Start the Band Viewer
    ChooseBandViewerSDR
    GUI_RETURN_CODE=$BANDVIEW_START_CODE
  ;;
  TestMenu_boot)
    # Start the Portsdown in the test equipment menu
    GUI_RETURN_CODE=207
  ;;
  SA_boot)
    # Start the spectrum analyser
    GUI_RETURN_CODE=170
  ;;
  Null_boot)
    # Exit the scheduler
    exit
  ;;
  TX_boot)
    # Start the Portsdown in Transmit Mode
    GUI_RETURN_CODE=199
  ;;
  RX_boot)
    # Start the Portsdown in Receive Mode
    GUI_RETURN_CODE=198
  ;;
  *)
    # Default to Portsdown
    GUI_RETURN_CODE=129
  ;;
esac

while [ "$GUI_RETURN_CODE" -gt 127 ] || [ "$GUI_RETURN_CODE" -eq 0 ];  do

echo "Calling App "$GUI_RETURN_CODE" from scheduler"
echo

  case "$GUI_RETURN_CODE" in
    0)
      /home/pi/portsdown/bin/portsdown5 # > /tmp/PortsdownGUI.log 2>&1
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    128)
      # Jump out of the loop
      break
    ;;
    129)
      /home/pi/portsdown/bin/portsdown5 # > /tmp/PortsdownGUI.log 2>&1
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    130)
      /home/pi/portsdown/bin/siggen
      GUI_RETURN_CODE="$?"
    ;;
    132)
      cd /home/pi
      /home/pi/update.sh -p
    ;;
    133)
      cd /home/pi
      /home/pi/update.sh -d
    ;;
    134)
      /home/pi/portsdown/bin/power_meter xy
      GUI_RETURN_CODE="$?"
    ;;
    135)
      cd /home/pi
      /home/pi/Langstone/run
      /home/pi/Langstone/stop
      PLUTOIP=$(get_config_var plutoip $PCONFIGFILE)
      ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "$PLUTOIP" >/dev/null 2>/dev/null
      # ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "pluto.local" >/dev/null 2>/dev/null
      timeout 2 sshpass -p analog ssh -o StrictHostKeyChecking=no root@"$PLUTOIP" 'PATH=/bin:/sbin:/usr/bin:/usr/sbin;reboot'
      sleep 2
      GUI_RETURN_CODE="129"
    ;;
    136)
      /home/pi/portsdown/bin/limeview
      GUI_RETURN_CODE="$?"
    ;;
    137)
      sleep 1
      /home/pi/portsdown/bin/power_meter
      GUI_RETURN_CODE="$?"
    ;;
    138)
      /home/pi/portsdown/bin/nf_meter
      GUI_RETURN_CODE="$?"
    ;;
    139)
      sleep 1
      /home/pi/portsdown/bin/sweeper
      GUI_RETURN_CODE="$?"
    ;;
    140)
      sleep 1
      /home/pi/portsdown/bin/airspyview
      GUI_RETURN_CODE="$?"
    ;;
    141)
      sleep 1
      /home/pi/portsdown/bin/rtlsdrview
      GUI_RETURN_CODE="$?"
    ;;
    142)
      sleep 1
      /home/pi/portsdown/bin/dmm
      GUI_RETURN_CODE="$?"
    ;;
    143)
      sleep 1
      /home/pi/portsdown/bin/plutoview
      GUI_RETURN_CODE="$?"
    ;;
    144)                                    # SDRplayview
      sleep 1
      RPISTATE="Not_Ready"
      while [[ "$RPISTATE" == "Not_Ready" ]]
      do
        DisplayMsg "Restarting SDRPlay Service\n\nThis may take up to 90 seconds"
        /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &

        sudo systemctl restart sdrplay

        DisplayMsg " "                      # Display Blank screen
        /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &

        lsusb | grep -q '1df7:'             # check for SDRPlay
        if [ $? != 0 ]; then                # Not detected
          DisplayMsg "Unable to detect SDRPlay\n\nResetting the USB Bus"
          /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
          sudo uhubctl -R -a 2              # So reset USB bus
          sleep 1

          lsusb | grep -q '1df7:'           # Check again
          if [ $? != 0 ]; then              # Not detected
            DisplayMsg "Unable to detect SDRPlay\n\nResetting the USB Bus"
            /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
            sudo uhubctl -R -a 2            # Try reset USB bus again
            sleep 1

            lsusb | grep -q '1df7:'         # Has that worked?
            if [ $? != 0 ]; then            # No
              DisplayMsg "Still Unable to detect SDRPlay\n\n\nCheck connections"
              /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
              sleep 2
              DisplayMsg " "                # Display Blank screen
              /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
            else
              RPISTATE="Ready"     
            fi
          else
            RPISTATE="Ready"
          fi
        else
          RPISTATE="Ready"
        fi
      done

      /home/pi/portsdown/bin/sdrplayview
      GUI_RETURN_CODE="$?"

      if [ $GUI_RETURN_CODE != 129 ] && [ $GUI_RETURN_CODE != 160 ]; then     # Not Portsdown and not shutdown
        GUI_RETURN_CODE=144                         # So restart sdrplayview        
      fi
    ;;
    145)                              # Langstone V2 Lime
      cd /home/pi
      /home/pi/Langstone/run_lime
      /home/pi/Langstone/stop_lime
      sleep 2
      GUI_RETURN_CODE="129"
    ;;
    146)                              # Langstone V2 Pluto
      cd /home/pi
      /home/pi/Langstone/run_pluto
      /home/pi/Langstone/stop_pluto
      PLUTOIP=$(get_config_var plutoip $PCONFIGFILE)
      ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "$PLUTOIP" >/dev/null 2>/dev/null
      # ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "pluto.local" >/dev/null 2>/dev/null
      timeout 2 sshpass -p analog ssh -o StrictHostKeyChecking=no root@"$PLUTOIP" 'PATH=/bin:/sbin:/usr/bin:/usr/sbin;reboot'
      sleep 2
      GUI_RETURN_CODE="129"
    ;;
    147)
      /home/pi/portsdown/bin/noise_meter
      GUI_RETURN_CODE="$?"
    ;;
    148)
      /home/pi/portsdown/bin/pluto_nf_meter
      GUI_RETURN_CODE="$?"
    ;;
    149)
      /home/pi/portsdown/bin/pluto_noise_meter
      GUI_RETURN_CODE="$?"
    ;;
    150)                              # SDRPlay Meteor Viewer
      RPISTATE="Not_Ready"
      while [[ "$RPISTATE" == "Not_Ready" ]]
      do
        DisplayMsg "Restarting SDRPlay Service\n\nThis may take up to 90 seconds"
        /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &

        sudo systemctl restart sdrplay

        DisplayMsg " "                      # Display Blank screen
        /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &

        lsusb | grep -q '1df7:'             # check for SDRPlay
        if [ $? != 0 ]; then                # Not detected
          DisplayMsg "Unable to detect SDRPlay\n\nResetting the USB Bus"
          /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
          sudo uhubctl -R -a 2              # So reset USB bus
          sleep 1

          lsusb | grep -q '1df7:'           # Check again
          if [ $? != 0 ]; then              # Not detected
            DisplayMsg "Unable to detect SDRPlay\n\nResetting the USB Bus"
            /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
            sudo uhubctl -R -a 2            # Try reset USB bus again
            sleep 1

            lsusb | grep -q '1df7:'         # Has that worked?
            if [ $? != 0 ]; then            # No
              DisplayMsg "Still Unable to detect SDRPlay\n\n\nCheck connections"
              /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
              sleep 2
              DisplayMsg " "                # Display Blank screen
              /home/pi/portsdown/scripts/single_screen_grab_for_web.sh &
            else
              RPISTATE="Ready"     
            fi
          else
            RPISTATE="Ready"
          fi
        else
          RPISTATE="Ready"
        fi
      done

      /home/pi/portsdown/bin/meteorview
      GUI_RETURN_CODE="$?"

      if [ $GUI_RETURN_CODE != 129 ] && [ $GUI_RETURN_CODE != 160 ]; then     # Not Portsdown and not shutdown
        GUI_RETURN_CODE=150                         # So restart meteorview        
      fi
    ;;
    151)
      /home/pi/portsdown/bin/limeviewng
      GUI_RETURN_CODE="$?"
    ;;
    152)
      /home/pi/portsdown/bin/nf_meterng
      GUI_RETURN_CODE="$?"
    ;;
    155)
      # Start the Preferred Band Viewer
      ChooseBandViewerSDR
      GUI_RETURN_CODE=$BANDVIEW_START_CODE
    ;;
    160)
      sleep 1
      sudo swapoff -a
      sudo shutdown now
      break
    ;;
    170)
      /home/pi/portsdown/bin/sa_sched
      GUI_RETURN_CODE="$?"
    ;;
    171)
      /home/pi/portsdown/bin/portsdown5 -b rx
      GUI_RETURN_CODE="$?"
    ;;
    172)
      /home/pi/portsdown/bin/portsdown5 -b rx
      GUI_RETURN_CODE="$?"
    ;;
    173)
      /home/pi/portsdown/bin/sweeper
      GUI_RETURN_CODE="$?"
    ;;
    174)
      /home/pi/portsdown/bin/power_meter
      GUI_RETURN_CODE="$?"
    ;;
    175)
      /home/pi/portsdown/bin/sa_sdr
      GUI_RETURN_CODE="$?"
    ;;
    176)
      /home/pi/portsdown/bin/sa_bv
      GUI_RETURN_CODE="$?"
    ;;
    177)
      /home/pi/portsdown/bin/sa_if
      GUI_RETURN_CODE="$?"
    ;;
    178)
      /home/pi/portsdown/bin/sa_if             # future parameter here
      GUI_RETURN_CODE="$?"
    ;;
    179)
      /home/pi/portsdown/bin/sa_bv             # future parameter here
      GUI_RETURN_CODE="$?"
    ;;
    181)
      /home/pi/portsdown/bin/picoview
      GUI_RETURN_CODE="$?"
    ;;
    192)
      PLUTOIP=$(get_config_var plutoip $PCONFIGFILE)
      ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "$PLUTOIP" >/dev/null 2>/dev/null
      # ssh-keygen -f "/home/pi/.ssh/known_hosts" -R "pluto.local" >/dev/null 2>/dev/null
      timeout 2 sshpass -p analog ssh -o StrictHostKeyChecking=no root@"$PLUTOIP" 'PATH=/bin:/sbin:/usr/bin:/usr/sbin;reboot'
      sleep 2
      sudo swapoff -a
      sudo reboot now
      break
    ;;
    193)
      # Rotate 7 inch display
      # Three scenarios:
      #  (1) No text in /boot/config.txt, so append it
      #  (2) Rotate text is in /boot/config.txt, so comment it out
      #  (3) Commented text in /boot/config.txt, so uncomment it

      # Test for Scenario 1
      if ! grep -q 'lcd_rotate=2' /boot/config.txt; then
        # No relevant text, so append it (Scenario 1)
        sudo sh -c 'echo "\n## Rotate 7 inch Display\nlcd_rotate=2\n" >> /boot/config.txt'
      else
        # Text exists, so see if it is commented or not
        TEST_STRING="#lcd_rotate=2"
        if ! grep -q -F $TEST_STRING /boot/config.txt; then
          # Scenario 2
          sudo sed -i '/lcd_rotate=2/c\#lcd_rotate=2' /boot/config.txt >/dev/null 2>/dev/null
        else
          # Scenario 3
          sudo sed -i '/#lcd_rotate=2/c\lcd_rotate=2' /boot/config.txt  >/dev/null 2>/dev/null
        fi
      fi

      # Make sure that scheduler reboots and does not repeat 7 inch rotation
      GUI_RETURN_CODE=192
      sleep 1
      sudo swapoff -a
      sudo reboot now
      break
    ;;
    197)
      /home/pi/ryde-build/configs/ryde_rx.sh  # Starts Ryde in a new process
      /home/pi/portsdown/bin/rydemon            # Blocks here until screen is touched
      GUI_RETURN_CODE="$?"
      sleep 1
      sudo killall -9 python3 >/dev/null 2>/dev/null
      sudo killall -9 longmynd >/dev/null 2>/dev/null
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    198)
      /home/pi/portsdown/bin/portsdown5 -b rx   # Start Portsdown in RX mode
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    199)
      /home/pi/portsdown/bin/portsdown5 -b tx   # Start Portsdown in TX mode
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    207)
      /home/pi/portsdown/bin/portsdown5 -b 7
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
    *)
      /home/pi/portsdown/bin/portsdown5
      GUI_RETURN_CODE="$?"
      sudo killall vlc >/dev/null 2>/dev/null
    ;;
  esac
done


