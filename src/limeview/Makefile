all:	limeview

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
#LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt  -lbcm_host -lpng16 -lm -ldl -lfftw3f -lLimeSuite -lfcgi  -rdynamic -fcommon
LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt -lpng16 -lm -ldl -lfftw3f -lLimeSuite -lfcgi  -rdynamic -fcommon

limeview: limeview.c 
	gcc -fdiagnostics-color -Wall -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS -pthread $(INCLUDEFLAGS2) -o  limeview \
		lime.c \
		fft.c \
		../common/graphics.c \
		../common/hardware.c \
		../common/timing.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_32.c \
		../common/buffer/buffer_circular.c \
        ../common/LMNspi/LMNspi.c \
		limeview.c $(LIBFLAGS2)

install:
	cp limeview ../../bin/

# -lbcm_host -ljpeg 
# 		screen.c \
# 		../common/temperature.c \
