#! /bin/bash

# Copy the user configs from /home/pi/configs
# to /home/pi/portsdown/configs/ overwriting factory settings

# Copy configs
cp -rf /home/pi/configs/*.txt /home/pi/portsdown/configs/

# Remove the old directory
rm -rf /home/pi/configs/

echo User configs restored



