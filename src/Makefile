all: ../bin/OT ../bin/rtl_fm ../bin/demo_rtlfm  ../bin/sendiq ../bin/tune ../bin/genSSB ../bin/demo_genSSB ../bin/OT4D ../bin/demo_gpio ../bin/gpio

LIBDIR=/usr/local/lib
LIBRPITX = /home/pi/librpitx
PP   = /home/pi/PixiePi/src
OT   = /home/pi/OrangeThunder/src
CSDRPATH= /home/pi/csdr/

INCLUDEDIR=/usr/local/include

CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable

CFLAGS  = -Wall -g -O2 -Wno-unused-variable -lrt -lpthread -lpigpio -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 

OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D
LDFLAGS = $(LIBRPITX)/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lpigpio -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 

CCP  = c++
CC   = cc
GCC  = gcc
LSDR = /usr/local/lib/librtlsdr.a

#../bin/OT : OT/OT.cpp OT4D/OT4D.cpp $(LIBRPITX)/src/librpitx.a ./lib/genSSB.h ./lib/rtlfm.h ./lib/gpioWrapper.h $(PP)/minIni/minIni.c
#	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT ./OT/OT.cpp ./lib/genSSB.h ./lib/rtlfm.h ./lib/gpioWrapper.h  $(PP)/minIni/minIni.c  $(LDFLAGS)

../bin/OT4D : OT4D/OT4D.cpp $(LIBRPITX)/src/librpitx.a ./OT4D/OT4D.h $(PP)/minIni/minIni.c $(PP)/lib/CAT817.h ./lib/gpioWrapper.h ./lib/rtlfm.h ./lib/genSSB.h $(PP)/minIni/minIni.c $(PP)/lib/DDS.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT4D ./OT4D/OT4D.cpp ./lib/genSSB.h  ./lib/rtlfm.h ./lib/gpioWrapper.h $(PP)/minIni/minIni.c $(PP)/lib/DDS.h $(LDFLAGS)

../bin/rtl_fm : rtl-sdr/rtl_fm.c rtl-sdr/convenience.c
	$(CC) $(OSC_CFLAGS) -o ../bin/rtl_fm ./rtl-sdr/rtl_fm.c rtl-sdr/convenience.c $(LDFLAGS)


../bin/demo_rtlfm : lib/demo_rtlfm.c  $(PP)/lib/CAT817.h lib/rtlfm.h $(PP)/lib/SSB.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/demo_rtlfm ./lib/demo_rtlfm.c  ./lib/rtlfm.h $(LDFLAGS)

../bin/demo_genSSB : lib/demo_genSSB.c  lib/genSSB.h $(PP)/lib/CAT817.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/demo_genSSB ./lib/demo_genSSB.c ./lib/genSSB.h $(LDFLAGS)

../bin/genSSB : lib/genSSB.cpp /home/pi/librpitx/src/librpitx.a  $(PP)/minIni/minIni.c  $(PP)/lib/SSB.h
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/genSSB ./lib/genSSB.cpp $(LDFLAGS)


../bin/demo_gpio	: lib/demo_gpio.c 
	$(GCC) -o ../bin/demo_gpio ./lib/demo_gpio.c $(CFLAGS)

../bin/gpio	: lib/gpio.cpp
	$(CCP) -o ../bin/gpio ./lib/gpio.cpp $(LDFLAGS)


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
