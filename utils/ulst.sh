#! /bin/bash

# Recompile LimeSDR toolbox

# set -x

/home/pi/portsdown/bin/limesdr_stopchannel


cd /home/pi/portsdown/src/limesdr_toolbox/

touch limesdr_util.c
touch limesdr_dvb.cpp
touch limesdr_send.c
touch limesdr_stopchannel.c
touch limesdr_dump.c
touch limesdr_forward.c

make -j 4 -O

make dvb

mv limesdr_send /home/pi/portsdown/bin/
mv limesdr_dump /home/pi/portsdown/bin/
mv limesdr_stopchannel /home/pi/portsdown/bin/
mv limesdr_forward /home/pi/portsdown/bin/
mv limesdr_dvb /home/pi/portsdown/bin/



