all:	sa_sched

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

INCLUDEFLAGS2=-lpthread -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I.. -fdiagnostics-color 
LIBFLAGS2=-L/opt/vc/lib -lbcm_host -lpthread  -ljpeg -lpng16 -lrt -lm -ldl  -lLimeSuite -lfcgi -rdynamic -fcommon -lwiringPi

sa_sched: sa_sched.c 
	gcc -Wall $(INCLUDEFLAGS2) -o  sa_sched \
		../common/hardware.c \
		sa_sched.c $(LIBFLAGS2) 

install:
	cp sa_sched ../../bin/
