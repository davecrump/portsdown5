all:	fb2png

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS2=-L/opt/vc/lib -lpthread -lrt  -lbcm_host -lpng16 -lm -ldl -lfcgi  -rdynamic -fcommon

fb2png: fb2png.c 
	gcc -fdiagnostics-color -Wall -std=gnu11 -D_GNU_SOURCE -DNEON_OPTS -pthread $(INCLUDEFLAGS2) -o  fb2png \
		../common/graphics.c \
		../common/font/font.c \
		fb2png.c $(LIBFLAGS2)

install:
	cp fb2png ../../bin/

# -lbcm_host -ljpeg 
# 		screen.c \
# 		../common/temperature.c \
