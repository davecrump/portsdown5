all:	msgbox

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS=-fdiagnostics-color -Wall  -march=native -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS

LIBFLAGS1=-lm -lc -pthread -lusb-1.0 -Wl,-Bstatic -Wl,-Bdynamic
LIBFLAGS2=-L/opt/vc/lib -lrt -ldl -lfcgi -rdynamic -lpng16 -fcommon

msgbox: msgbox.c 
	gcc $(INCLUDEFLAGS) -o  msgbox \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_24.c \
		../common/font/dejavu_sans_26.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_30.c \
		../common/font/dejavu_sans_32.c \
		../common/font/dejavu_sans_72.c \
		../common/timing.c \
		../common/graphics.c \
		../common/hardware.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/utils.c \
		msgbox.c $(LIBFLAGS1) $(LIBFLAGS2) 

#install:
#	cp msgbox ../../bin/

# removing -O3 from INCLUDEFLAGS solved fb2png corrupt png problem 		sdrplayfft.c \-lsdrplay_api 
# -lfftw3 -lwiringPi 

