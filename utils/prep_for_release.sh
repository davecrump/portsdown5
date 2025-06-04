#! /bin/bash

# Copy the user configs to /home/pi/configs
# and replace with factory configs

# Make the new directory
rm -rf /home/pi/configs
mkdir /home/pi/configs/

# Copy in the configs
cp -r /home/pi/portsdown/configs/*.txt /home/pi/configs/

# Delete the old configs and backups
rm /home/pi/portsdown/configs/*.txt
rm /home/pi/portsdown/configs/*.bak

# Replace the build folder with factory configs

cp -rf /home/pi/portsdown/configs/factory/*.txt /home/pi/portsdown/configs/

echo User configs saved, factory configs restored



