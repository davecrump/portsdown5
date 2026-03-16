all:	basicRX

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132

LIBFLAGS2=-lstdc++ -llimesuiteng-legacyapi 

basicRX: basicRX.cpp 
	g++ -fdiagnostics-color -Wall -o  basicRX \
		basicRX.cpp $(LIBFLAGS2)

