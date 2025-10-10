all:	sweeper

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I.. -fdiagnostics-color
LIBFLAGS2=-L/opt/vc/lib -lpng16 -lpthread  -ljpeg -lrt -lm -ldl -lfftw3f -lwiringPi -lLimeSuite -lfcgi -rdynamic -fcommon 

sweeper: sweeper.c 
	gcc -Wall -std=gnu11 -D_GNU_SOURCE $(INCLUDEFLAGS2) -o  sweeper \
		../common/mcp3002.c \
		../common/graphics.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/timing.c \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_32.c \
		../common/font/dejavu_sans_36.c \
		sweeper.c $(LIBFLAGS2) 

install:
	cp sweeper ../../bin/