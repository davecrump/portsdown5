all:	power_meter

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I.. -fdiagnostics-color
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpthread  -ljpeg -lpng16 -lrt -lm -ldl -lwiringPi -lfcgi -rdynamic -fcommon 

#power_meter: mcp3002.h mcp3002.c power_meter.c 
power_meter: power_meter.c 
	gcc -Wall $(INCLUDEFLAGS2) -o  power_meter \
		../common/mcp3002.c \
		../common/hardware.c \
		../common/graphics.c \
		../common/timing.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_32.c \
		../common/font/dejavu_sans_36.c \
		power_meter.c $(LIBFLAGS2) 

install:
	cp power_meter ../../bin/