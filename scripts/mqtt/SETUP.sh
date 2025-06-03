#!/usr/bin/bash

# Initial setup by DL5OCD

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


echo "Initial setup to set the CALLSIGN for the Pluto (new F5OEO FW only!)..."
echo "------------------------------------------------------------------------"
echo "Please be shure to set the variables CALLSIGN, PLUTOIPMQTT and CMD_ROOT in config-tx.ini !!!!!"
echo "Install Mosquitto before you proceed !!!!!"
echo "Be shure, that the Pluto is reachable."
echo "When everything is ready, press Enter..."

read

ping -c 5 $PLUTOIP


mosquitto_pub -t cmd/pluto/call -m $CALLSIGN -h $PLUTOIP
echo Reboot the Pluto.
echo You can close this window now. Press Enter...
read
exit
