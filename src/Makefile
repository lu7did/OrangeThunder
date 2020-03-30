all: ../bin/OT 

OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D

CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable
LDFLAGS = /home/pi/librpitx/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lwiringPiDev -lpigpio 
CFLAGS  = -Wall -g -O2 -Wno-unused-variable
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable
PP = /home/pi/PixiePi/src

CCP = c++
CC = cc
GCC = gcc


../bin/OT : OT/OT.cpp /home/pi/librpitx/src/librpitx.a $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT ./OT/OT.cpp $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c  $(LDFLAGS)

clean:
	rm -r  ../bin/OT

install: all
	install -m 0755 ../bin/OT  /usr/bin
