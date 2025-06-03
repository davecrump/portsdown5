all:	sa_if

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I.. -fdiagnostics-color 
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpthread  -ljpeg -lpng16 -lrt -lm -ldl  -lLimeSuite -lfcgi -rdynamic -fcommon -lwiringPi

sa_if: sa_if.c 
	gcc -Wall $(INCLUDEFLAGS2) -o  sa_if \
		lime.c \
		../common/graphics.c \
		../common/timing.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/mcp3002.c \
		../common/hardware.c \
        ../common/temperature.c \
        ../common/siggen.c \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_32.c \
		../common/buffer/buffer_circular.c \
		sa_if.c $(LIBFLAGS2) 

install:
	cp sa_if ../../bin/
