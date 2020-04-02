all: ../bin/OT ../bin/pcm_min ../bin/test_rtlsdr ../bin/rtl_fm

LIBDIR=/usr/local/lib
INCLUDEDIR=/usr/local/include
OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D
CSDRPATH= /home/pi/csdr/
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable
LDFLAGS = /home/pi/librpitx/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lwiringPiDev -lpigpio  -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 
CFLAGS  = -Wall -g -O2 -Wno-unused-variable -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM
PP = /home/pi/PixiePi/src

CCP = c++
CC = cc
GCC = gcc
LSDR = /usr/local/lib/librtlsdr.a


../bin/OT : OT/OT.cpp /home/pi/librpitx/src/librpitx.a $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c
	$(CCP) $(OSC_CFLAGS) $(CXXFLAGS) -o ../bin/OT ./OT/OT.cpp $(PP)/lib/SSB.h $(PP)/lib/RPI.h $(PP)/minIni/minIni.c $(LDFLAGS)

../bin/rtl_fm : lib/rtl_fm.c lib/convenience.c
	$(CC) $(OSC_CFLAGS) -o ../bin/rtl_fm ./lib/rtl_fm.c lib/convenience.c $(LDFLAGS)

../bin/pcm_min : lib/pcm_min.c 
	$(CC) $(OSC_CFLAGS) $(CFLAGS) -o ../bin/pcm_min ./lib/pcm_min.c  $(LDFLAGS)

../bin/test_rtlsdr : lib/test_rtlsdr.c 
	$(CC) $(OSC_CFLAGS) $(CFLAGS) -o ../bin/test_rtlsdr ./lib/test_rtlsdr.c  $(LDFLAGS)

clean:
	rm -r  ../bin/OT
	rm -r  ../bin/pcm_min

install: all
	install -m 0755 ../bin/OT  /usr/bin
	install -m 0755 ../bin/pcm_min /usr/bin
