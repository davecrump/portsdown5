all:	portsdown5

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt -lpng16 -lm -ldl -rdynamic -lfcgi -fcommon

portsdown5: portsdown5.c 
	gcc -fdiagnostics-color -Wall $(INCLUDEFLAGS2) -o  portsdown5 \
		lmrx_utils.c \
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
		portsdown5.c $(LIBFLAGS2) 

#install:
	cp portsdown5 ../../bin/


