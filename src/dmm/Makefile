all:	dmm

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpthread -lpng16  -ljpeg -lrt -lm -ldl -lwiringPi -lfcgi -rdynamic -fcommon

dmm: dmm.c 
	gcc -fdiagnostics-color -Wall $(INCLUDEFLAGS2) -o  dmm \
		../common/graphics.c \
		../common/timing.c \
		../common/ffunc.c \
		../common/ffunc.h \
		../common/utils.c \
		../common/hardware.c \
		../common/font/font.c \
		../common/font/dejavu_sans_18.c \
		../common/font/dejavu_sans_20.c \
		../common/font/dejavu_sans_22.c \
		../common/font/dejavu_sans_28.c \
		../common/font/dejavu_sans_32.c \
		../common/font/dejavu_sans_72.c \
		../common/buffer/buffer_circular.c \
        ../common/LMNspi/LMNspi.c \
		dmm.c $(LIBFLAGS2) 

install:
	cp dmm ../../bin/
