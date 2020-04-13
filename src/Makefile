all: ../bin/OT ../bin/rtl_fm ../bin/demo_rtlfm  ../bin/sendiq ../bin/tune ../bin/genSSB ../bin/demo_genSSB ../bin/OT4D

LIBDIR=/usr/local/lib
LIBRPITX = /home/pi/librpitx
PP   = /home/pi/PixiePi/src
OT   = /home/pi/OrangeThunder/src
CSDRPATH= /home/pi/csdr/

INCLUDEDIR=/usr/local/include

CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable

CFLAGS  = -Wall -g -O2 -Wno-unused-variable -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 

OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D
#LDFLAGS = $(LIBRPITX)/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lwiringPiDev -lpigpio  -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 
LDFLAGS = $(LIBRPITX)/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 

CCP  = c++
CC   = cc
GCC  = gcc
LSDR = /usr/local/lib/librtlsdr.a

#../bin/sendiq : rpitx/sendiq.cpp $(LIBRPITX)/src/librpitx.a
#	$(CCP) $(CXXFLAGS) -o ../bin/sendiq rpitx/sendiq.cpp  $(LDFLAGS)
#
#../bin/tune : rpitx/tune.cpp $(LIBRPITX)/src/librpitx.a
#	$(CCP) $(CXXFLAGS) -o ../bin/tune rpitx/tune.cpp  $(LDFLAGS)

../bin/OT : OT/OT.cpp $(LIBRPITX)/src/librpitx.a $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT ./OT/OT.cpp $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c  $(LDFLAGS)

../bin/OT4D : OT4D/OT4D.cpp $(LIBRPITX)/src/librpitx.a $(PP)/lib/SSB.h $(PP)/minIni/minIni.c $(PP)/lib/CAT817.h ./lib/rtlfm.h ./lib/genSSB.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT4D ./OT4D/OT4D.cpp ./lib/genSSB.h  ./lib/rtlfm.h $(LDFLAGS)

../bin/demo_rtlfm : lib/demo_rtlfm.c  $(PP)/lib/CAT817.h 
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/demo_rtlfm ./lib/demo_rtlfm.c  $(LDFLAGS)

../bin/demo_genSSB : lib/demo_genSSB.c  lib/genSSB.h $(PP)/lib/CAT817.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/demo_genSSB ./lib/demo_genSSB.c ./lib/genSSB.h $(LDFLAGS)

../bin/genSSB : lib/genSSB.cpp /home/pi/librpitx/src/librpitx.a  $(PP)/lib/RPI.h $(PP)/minIni/minIni.c
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/genSSB ./lib/genSSB.cpp $(LDFLAGS)

../bin/rtl_fm : rtl-sdr/rtl_fm.c rtl-sdr/convenience.c
	$(CC) $(OSC_CFLAGS) -o ../bin/rtl_fm ./rtl-sdr/rtl_fm.c rtl-sdr/convenience.c $(LDFLAGS)

clean:
	rm -r  ../bin/OT
	rm -r  ../bin/OT4D
	rm -r  ../bin/rtl_fm
	rm -r  ../bin/demo_rtlfm
	rm -r  ../bin/demo_genSSB

install: all
	install -m 0755 ../bin/OT  /usr/bin
	install -m 0755 ../bin/OT4D  /usr/bin
	install -m 0755 ../bin/demo_rtlfm  /usr/bin
	install	-m 0755 ../bin/demo_genSSB  /usr/bin
	install	-m 0755 ../bin/rtl_fm  /usr/bin
