all:	siggentouch4

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpng16 -lpthread -ljpeg -lrt -lm -ldl -lfftw3f -lwiringPi -liio -lLimeSuite -lfcgi -rdynamic -fcommon

siggentouch4: siggen5.c 
	gcc -Wall $(INCLUDEFLAGS2) -o  siggen \
		../common/graphics.c \
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
		../common/hardware.c \
		../common/ffunc.c \
		../common/ffunc.h \
		siggen5.c $(LIBFLAGS2) 

install:
	cp siggen ../../bin/