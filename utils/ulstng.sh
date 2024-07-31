#! /bin/bash

# Recompile LimeSDR toolbox with LimeSuiteNG

# set -x


cd /home/pi/portsdown/src/limesdr_toolboxng/

touch limesdr_utilng.c
touch limesdr_dvbng.cpp
touch limesdr_send.c
touch limesdr_stopchannel.c
touch limesdr_dump.c
touch limesdr_forward.c

make -j 4 -O

make dvb

mv limesdr_dvbng /home/pi/portsdown/bin/
mv limesdr_stopchannel /home/pi/portsdown/bin/



