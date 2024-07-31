FLAGS = -Wall -llimesuiteng -g -O2 -Wl,-rpath,/home/pi/LimeSuiteNG/build/lib/ -L/home/pi/LimeSuiteNG/build/lib/

all:
	gcc -o limesdr_dump limesdr_dump.c limesdr_utilng.c $(FLAGS)
	gcc -o limesdr_send limesdr_send.c limesdr_utilng.c $(FLAGS)
	gcc -o limesdr_stopchannel limesdr_stopchannel.c $(FLAGS)
	gcc -o limesdr_forward limesdr_forward.c limesdr_utilng.c $(FLAGS)

dvb:
	g++ -o limesdr_dvbng limesdr_dvbng.cpp limesdr_utilng.c $(FLAGS) -lm -lrt -lfftw3 -lpthread ./libdvbmod/libdvbmod/lib/libdvbmod.a 

