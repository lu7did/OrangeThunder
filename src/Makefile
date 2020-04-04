all: ../bin/OT ../bin/rtl_fm ../bin/demo_rtlfm 

LIBDIR=/usr/local/lib
LIBRPITX = /home/pi/librpitx

INCLUDEDIR=/usr/local/include

CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable

CFLAGS  = -Wall -g -O2 -Wno-unused-variable -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 

OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D
CSDRPATH= /home/pi/csdr/
LDFLAGS = /home/pi/librpitx/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lwiringPiDev -lpigpio  -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 

PP   = /home/pi/PixiePi/src
OT   = /home/pi/OrangeThunder/src
CCP  = c++
CC   = cc
GCC  = gcc
LSDR = /usr/local/lib/librtlsdr.a


../bin/OT : OT/OT.cpp $(LIBRPITX)/src/librpitx.a $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT ./OT/OT.cpp $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c  $(LDFLAGS)

../bin/rtl_fm : rtl-sdr/rtl_fm.c rtl-sdr/convenience.c
	$(CC) $(OSC_CFLAGS) -o ../bin/rtl_fm ./rtl-sdr/rtl_fm.c rtl-sdr/convenience.c $(LDFLAGS)

../bin/demo_rtlfm : lib/demo_rtlfm.c 
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/demo_rtlfm ./lib/demo_rtlfm.c  $(LDFLAGS)

#---- Staffing crypt
#../bin/test_rtlfm2 : lib/test_rtlfm2.c 
#	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/test_rtlfm2 ./lib/test_rtlfm2.c  $(LDFLAGS)
#../bin/pcm_min : lib/pcm_min.c 
#	$(CC) $(OSC_CFLAGS) $(CFLAGS) -o ../bin/pcm_min ./lib/pcm_min.c  $(LDFLAGS)
#
#---------------
clean:
	rm -r  ../bin/OT
	rm -r  ../bin/rtl_fm
	rm -r  ../bin/demo_rtlfm

install: all
	install -m 0755 ../bin/OT  /usr/bin
	install -m 0755 ../bin/rtl_fm  /usr/bin
	install -m 0755 ../bin/demo_rtlfm  /usr/bin
