/*
 * OT.h (configuration and header file)
 * Raspberry Pi based USB experimental SSB Generator
 * Experimental version largely modelled after Generator.java by Takafumi INOUE (JI3GAB)
 *---------------------------------------------------------------------
 * This program operates as a controller for a Raspberry Pi to control
 * a raspberry pi based  transceiver using TAPR hardware.
 * Project at http://www.github.com/lu7did/OrangeThunder
 *---------------------------------------------------------------------
 *
 * Created by Pedro E. Colla (lu7did@gmail.com)
 * Code excerpts from several packages:
 *    Adafruit's python code for CharLCDPlate
 *    tune.cpp from rpitx package by Evariste Courjaud F5OEO
 *    sendiq.cpp from rpitx package (also) by Evariste Coujaud (F5EOE)
 *    wiringPi library (git clone git://git.drogon.net/wiringPi)
 *    iambic-keyer (https://github.com/n1gp/iambic-keyer)
 *    log.c logging facility by  rxi https://github.com/rxi/log.c
 *    minIni configuration management package by Compuphase https://github.com/compuphase/minIni/tree/master/dev
*    tinyalsa https://github.com/tinyalsa/tinyalsa
 * Also libraries
 *    librpitx by  Evariste Courjaud (F5EOE)
 *    libcsdr by Karol Simonyi (HA7ILM) https://github.com/compuphase/minIni/tree/master/dev
 *
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301, USA.
 */

#define CATBAUD 	4800
#define CAT_PORT        "/tmp/ttyv0"
#define PTT_FIFO       	"/tmp/ptt_fifo"
#define _NOP        	(byte)0

#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))
#define BOOL2CHAR(x)  (x==true ? "True" : "False")


//*--- AGC performance parameters

#define AGC_REF          1.0
#define AGC_MAX          5.0
#define AGC_ALPHA        0.5
#define AGC_GAIN         1.0
#define AGC_LEVEL       0.80

//*--- VOX performance paramenters

#define VOX_TIMEOUT      2.0
#define VOX_MIN          0.0
#define VOX_MAX         10.0



#define ONESEC       1

#define CHANNEL      1
#define SOUNDHW    "Loopback"
#define AFRATE   48000

#define MAXGPIO 	32

#define GPIO_DDS     4
#define GPIO_PTT    12
#define GPIO_KEYER  16
#define GPIO_COOLER 19
#define GPIO_AUX    20
#define GPIO_PA     21
#define GPIO_CLK    17
#define GPIO_DT     18
#define GPIO_SW     27

#define GPIO_IN      0
#define GPIO_OUT     1

#define GPIO_PUP     1
#define GPIO_PDN     0
#define GPIO_NLP     0
#define GPIO_LP      1

#define BUFSIZE     1024
#define RTLSIZE     2048
#define GENSIZE     2048
//#define IQBURST     4096


#define IQSR        6000
#define DECIMATION     1

#define UNDEFINED     -1

#define NODEBUG        0
#define MINOR          1
#define DEBUG          2

//*--- Master System Word (MSW)

#define CMD       0B00000001
#define GUI       0B00000010
#define PTT       0B00000100
#define VOX       0B00001000   //redefinition VOX == DRF
#define RIT       0B00010000   //redefinition DOG == RIT
#define RUN       0B00100000
#define RETRY     0B01000000
#define BCK       0B10000000
#define QUIET     0B10000000

#define MLSB      0x00
#define MUSB      0x01
#define MCW       0x02
#define MCWR      0x03
#define MAM       0x04
#define MWFM      0x06
#define MFM       0x08
#define MDIG      0x0A
#define MPKT      0x0C


typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

#define CFGFILE    "./OT.cfg"

#define MINSWPUSH  10
#define MAXSWPUSH  2000

#define DDS_MAXLEVEL 7
// GPIO pins
#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

#define MINSWPUSH  10
#define MAXSWPUSH  2000
#define MINENCLAP   2

