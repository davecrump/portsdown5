all:	noise_meter

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpthread -lpng16  -ljpeg -lrt -lm -ldl -lfftw3f -lLimeSuite -lfcgi -rdynamic  -fcommon -lwiringPi

noise_meter: noise_meter.c 
	gcc -Wall -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS -pthread $(INCLUDEFLAGS2) -o  noise_meter \
		lime.c \
		fft.c \
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
		../common/font/dejavu_sans_72.c \
		../common/buffer/buffer_circular.c \
		../common/hardware.c \
		../common/temperature.c \
		../common/utils.c \
		noise_meter.c $(LIBFLAGS2)

install:
	cp noise_meter ../../bin/

