all:	img2fb

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
#LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt  -lbcm_host -lpng16 -lm -ldl -lfcgi  -rdynamic -fcommon
LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt -lpng16 -lm -ldl -lfcgi  -rdynamic -fcommon

img2fb: img2fb.c 
	gcc -fdiagnostics-color -Wall -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS -pthread $(INCLUDEFLAGS2) -o  img2fb \
		../common/graphics.c \
		../common/font/font.c \
		img2fb.c $(LIBFLAGS2)

install:
	cp img2fb ../../bin/


