all: ../bin/rtl_fm ../bin/demo_rtlfm  ../bin/genSSB ../bin/demo_genSSB ../bin/OT4D ../bin/demo_gpio ../bin/gpioWrapper ../bin/decodeFT8 ../bin/decimator ../bin/demo_decodeFT8 ../bin/demo_craddle ../bin/pift8 ../bin/sendRF

CCP  = c++
CC   = cc
GCC  = gcc

CFLAGS  = -Wall -g -O3 -Wno-unused-variable -lrt -lpthread -lpigpio -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
LIBRPITX = /home/pi/librpitx
CXYFLAGS = -std=c++11 -Wall -g -O3 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 
LDFLAGS = $(LIBRPITX)/src/librpitx.a -lm -lrt -lpthread -lwiringPi -lpigpio -lasound -lusb-1.0 -lrtlsdr -L$(LIBDIR) 




LIBDIR=/usr/local/lib
PP   = /home/pi/PixiePi/src
OT   = /home/pi/OrangeThunder/src
CSDRPATH= /home/pi/csdr/
FT8LIB=/home/pi/ft8_lib
INCLUDEDIR=/usr/local/include

CXXRPITX = -std=c++11 -Wall -g -O3 -Wno-unused-variable
LDRPITX = -lm

CXXFLAGS = -std=c++11  -g -O2 -Wno-unused-variable

CXXFLAGS = -std=c++11 -Wall -g -O2 -Wno-unused-variable -DLIBCSDR_GPL -DUSE_FFTW -DUSE_IMA_ADPCM -I$(INCLUDEDIR) -I/usr/include/libusb-1.0 

OSC_CFLAGS=-DOSCILLATOR_Z -DOSCILLATOR_D

LSDR = /usr/local/lib/librtlsdr.a

../bin/OT4D : OT4D/OT4D.cpp $(LIBRPITX)/src/librpitx.a OT4D/OT4D.h $(PP)/minIni/minIni.c lib/CAT817.h lib/rtlfm.h lib/genSSB.h $(PP)/minIni/minIni.c 
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/OT4D OT4D/OT4D.cpp  $(PP)/minIni/minIni.c  $(LDFLAGS)

../bin/rtl_fm : rtl-sdr/rtl_fm.c rtl-sdr/convenience.c
	$(CC) $(OSC_CFLAGS) $(CFLAGS)  -o ../bin/rtl_fm ./rtl-sdr/rtl_fm.c rtl-sdr/convenience.c $(LDFLAGS)

../bin/demo_rtlfm : lib/demo_rtlfm.c  lib/rtlfm.h lib/SSB.h
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/demo_rtlfm lib/demo_rtlfm.c  lib/rtlfm.h $(LDFLAGS)

../bin/demo_genSSB : lib/demo_genSSB.c  lib/genSSB.h lib/CAT817.h
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/demo_genSSB lib/demo_genSSB.c lib/genSSB.h $(LDFLAGS)

../bin/genSSB : lib/genSSB.cpp $(LIBRPITX)/src/librpitx.a  $(PP)/minIni/minIni.c  lib/SSB.h
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/genSSB lib/genSSB.cpp $(LDFLAGS)


#../bin/demo_gpio	: lib/demo_gpio.c 
#	$(GCC) -o ../bin/demo_gpio ./lib/demo_gpio.c $(CFLAGS)
#
#../bin/gpioWrapper	: lib/gpioWrapper.cpp
#	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -g -o ../bin/gpioWrapper ./lib/gpioWrapper.cpp $(LDFLAGS)

../bin/demo_genDDS     : lib/demo_genDDS.cpp lib/genDDS.h
	$(CCP) -o ../bin/demo_genDDS ./lib/demo_genDDS.cpp ./lib/genDDS.h $(LDFLAGS)

../bin/demo_decodeFT8     : lib/demo_decodeFT8.cpp lib/decodeFT8.h lib/cabrillo.h
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/demo_decodeFT8 ./lib/demo_decodeFT8.cpp ./lib/decodeFT8.h $(LDFLAGS)

../bin/decimator     : lib/decimator.cpp 
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/decimator ./lib/decimator.cpp  $(LDFLAGS)

../bin/decodeFT8     : lib/decodeFT8.cpp lib/libFT8.h
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/decodeFT8 ./lib/decodeFT8.cpp $(FT8LIB)/ft8/unpack.cpp $(FT8LIB)/ft8/ldpc.cpp $(FT8LIB)/ft8/decode.cpp $(FT8LIB)/ft8/constants.cpp $(FT8LIB)/ft8/encode.cpp $(FT8LIB)/common/wave.cpp $(FT8LIB)/fft/kiss_fftr.c  $(FT8LIB)/fft/kiss_fft.c $(FT8LIB)/ft8/text.cpp $(LDFLAGS)

#../bin/demo_craddle	: lib/demo_craddle.cpp
#	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/demo_craddle ./lib/demo_craddle.cpp $(LDFLAGS)

../bin/pift8	: rpitx/pift8/pift8.cpp
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/pift8 ./rpitx/pift8/pift8.cpp $(FT8LIB)/ft8/pack.cpp $(FT8LIB)/ft8/unpack.cpp $(FT8LIB)/ft8/ldpc.cpp $(FT8LIB)/ft8/decode.cpp $(FT8LIB)/ft8/constants.cpp $(FT8LIB)/ft8/encode.cpp $(FT8LIB)/common/wave.cpp $(FT8LIB)/fft/kiss_fftr.c  $(FT8LIB)/fft/kiss_fft.c $(FT8LIB)/ft8/text.cpp $(LDFLAGS)

../bin/sendRF	: lib/sendRF.cpp /home/pi/librpitx/src/librpitx.a
	$(CCP) $(OSC_CFLAGS) $(CXYFLAGS) -o ../bin/sendRF lib/sendRF.cpp  $(LDFLAGS)



clean:
	rm -r  ../bin/OT4D
#	rm -r  ../bin/gpioWrapper
	rm -r  ../bin/rtl_fm
	rm -r  ../bin/genSSB
	rm -r  ../bin/demo_rtlfm
	rm -r  ../bin/decimator
	rm -r  ../bin/demo_rtlfm
#	rm -r  ../bin/demo_gpio
	rm -r  ../bin/demo_decodeFT8
	rm -r  ../bin/demo_genSSB
	rm -r  ../bin/decodeFT8
	rm -r  ../bin/sendRF
	rm -r  ../bin/pift8

install: all
	install -m 0755 ../bin/OT4D  /usr/bin
#	install -m 0755 ../bin/gpioWrapper  /usr/bin
	install	-m 0755 ../bin/rtl_fm  /usr/bin
	install	-m 0755 ../bin/genSSB  /usr/bin
	install -m 0755 ../bin/demo_rtlfm  /usr/bin
	install -m 0755 ../bin/demo_decodeFT8  /usr/bin
#	install -m 0755 ../bin/demo_gpio  /usr/bin
	install	-m 0755 ../bin/decimator  /usr/bin
	install	-m 0755 ../bin/pift8  /usr/bin
	install	-m 0755 ../bin/decodeFT8  /usr/bin
	install	-m 0755 ../bin/sendRF  /usr/bin
	install	-m 0755 ../python/gpioset.py  /usr/bin
	install	-m 0755 ../bin/demo_genSSB  /usr/bin
