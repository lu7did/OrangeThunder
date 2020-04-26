/*
 * decimator.cpp
 * Raspberry Pi based USB experimental SSB Generator
 * simple decimator component
 *---------------------------------------------------------------------
 * This program operates as a decimator to allow reduction of 48000 samples/sec
 * to 12000 samples/sec. Other ratios might be attempted
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


#include <unistd.h>
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <cstdlib>      // for std::rand() and std::srand()
#include <ctime>        // for std::time()
#include <iostream>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <pigpio.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits.h>
#include <functional>

#include <sys/types.h>
#include <sys/stat.h>

//#include "/home/pi/PixiePi/src/lib/SSB.h"
#include "/home/pi/librpitx/src/librpitx.h"
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include "/home/pi/OrangeThunder/src/OT4D/transceiver.h"
#include "/home/pi/PixiePi/src/lib/Decimator.h"

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="decimator";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


byte      TRACE=0x00;
typedef unsigned char byte;
typedef bool boolean;

float     decimation_factor=4.0;
int       ax;

byte      MSW=0;
short     *buffer_i16;
short     *buffer_o16;
float*    a;
Decimator *d;

//--------------------------[System Word Handler]---------------------------------------------------
// getSSW Return status according with the setting of the argument bit onto the SW
//--------------------------------------------------------------------------------------------------
bool getWord (unsigned char SysWord, unsigned char v) {

  return SysWord & v;

}
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------
void setWord(unsigned char* SysWord,unsigned char v, bool val) {

  *SysWord = ~v & *SysWord;
  if (val == true) {
    *SysWord = *SysWord | v;
  }

}

//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\ Usage:  \n\ decimator [-r decimation factor] [-t tracelevel]\n",PROGRAMID,PROG_VERSION,PROG_BUILD);

} /* end function print_usage */

//---------------------------------------------------------------------------------
// terminate
//---------------------------------------------------------------------------------
static void terminate(int num)
{
    setWord(&MSW,RUN,false);
    (TRACE>=0x00 ? fprintf(stderr,"%s: Caught TERM signal(%x) - Terminating \n",PROGRAMID,num) : _NOP);
}
//---------------------------------------------------------------------------------
// main 
//---------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

        (TRACE >= 0x02 ? fprintf(stderr,"%s:main() About to enter argument parsing\n",PROGRAMID) : _NOP); 
	while(1)
	{
		ax = getopt(argc, argv, "r:t:h");
                if(ax == -1) 
		{
			if(ax) break;
			else ax='h'; //print usage and exit
		}

	
		switch(ax)
		{
		case 't': // AGC Maxlevel
			TRACE=atoi(optarg);
   		        (TRACE >= 0x01 ? fprintf(stderr,"%s: TRACE=%d\n",PROGRAMID,TRACE) : _NOP);
			break;
                case 'r': // Debug level
                        decimation_factor = atof(optarg);
                        (TRACE>=0x01 ? fprintf(stderr,"%s: decimation_factor=%4.1f\n",PROGRAMID,decimation_factor) : _NOP);
                        break;
		case -1:
            	        break;
		case 'h':
			if (isprint(optopt) ) {
 			   fprintf(stderr, "%s: unknown option `-%c'.\n",PROGRAMID,optopt);
 			} else 	{
		           fprintf(stderr, "%s: unknown option character `\\x%x'.\n",PROGRAMID,optopt);
			}
			print_usage();
			exit(1);
			break;
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

        (TRACE>=0x01 ? fprintf(stderr,"%s:main(): Trap handler initialization\n",PROGRAMID) : _NOP);
	for (int i = 0; i < 64; i++) {
           struct sigaction sa;
           std::memset(&sa, 0, sizeof(sa));
           sa.sa_handler = terminate;
           sigaction(i, &sa, NULL);
        }

    a=(float*) malloc(96*sizeof(float));
    d = new Decimator(a, 83, decimation_factor);

/*
     Kaiser Window FIR Filter
     Passband: 0.0 - 3000.0 Hz
     Order: 83
     Transition band: 3000.0 Hz
     Stopband attenuation: 80.0 dB
*/     


    a[0] =	-1.7250879E-5f;
    a[1] =	-4.0276995E-5f;
    a[2] =	-5.6314686E-5f;
    a[3] =	-4.0164417E-5f;
    a[4] =	3.0053454E-5f;
    a[5] =	1.5370155E-4f;
    a[6] =	2.9180944E-4f;
    a[7] =	3.6717512E-4f;
    a[8] =	2.8903902E-4f;
    a[9] =	3.1934875E-11f;
    a[10] =	-4.716546E-4f;
    a[11] =	-9.818495E-4f;
    a[12] =	-0.001290066f;
    a[13] =	-0.0011395542f;
    a[14] =	-3.8172887E-4f;
    a[15] =	9.0173044E-4f;
    a[16] =	0.0023420234f;
    a[17] =	0.003344623f;
    a[18] =	0.003282209f;
    a[19] =	0.0017731993f;
    a[20] =	-0.0010558856f;
    a[21] =	-0.004450674f;
    a[22] =	-0.0071515352f;
    a[23] =	-0.007778209f;
    a[24] =	-0.0053855875f;
    a[25] =	-2.6561373E-10f;
    a[26] =	0.0070972904f;
    a[27] =	0.013526209f;
    a[28] =	0.016455514f;
    a[29] =	0.013607533f;
    a[30] =	0.0043148645f;
    a[31] =	-0.009761283f;
    a[32] =	-0.02458954f;
    a[33] =	-0.03455451f;
    a[34] =	-0.033946108f;
    a[35] =	-0.018758629f;
    a[36] =	0.011756961f;
    a[37] =	0.054329403f;
    a[38] =	0.10202855f;
    a[39] =	0.14574805f;
    a[40] =	0.17644218f;
    a[41] =	0.18748334f;
    a[42] =	0.17644218f;
    a[43] =	0.14574805f;
    a[44] =	0.10202855f;
    a[45] =	0.054329403f;
    a[46] =	0.011756961f;
    a[47] =	-0.018758629f;
    a[48] =	-0.033946108f;
    a[49] =	-0.03455451f;
    a[50] =	-0.02458954f;
    a[51] =	-0.009761283f;
    a[52] =	0.0043148645f;
    a[53] =	0.013607533f;
    a[54] =	0.016455514f;
    a[55] =	0.013526209f;
    a[56] =	0.0070972904f;
    a[57] =	-2.6561373E-10f;
    a[58] =	-0.0053855875f;
    a[59] =	-0.007778209f;
    a[60] =	-0.0071515352f;
    a[61] =	-0.004450674f;
    a[62] =	-0.0010558856f;
    a[63] =	0.0017731993f;
    a[64] =	0.003282209f;
    a[65] =	0.003344623f;
    a[66] =	0.0023420234f;
    a[67] =	9.0173044E-4f;
    a[68] =	-3.8172887E-4f;
    a[69] =	-0.0011395542f;
    a[70] =	-0.001290066f;
    a[71] =	-9.818495E-4f;
    a[72] =	-4.716546E-4f;
    a[73] =	3.1934875E-11f;
    a[74] =	2.8903902E-4f;
    a[75] =	3.6717512E-4f;
    a[76] =	2.9180944E-4f;
    a[77] =	1.5370155E-4f;
    a[78] =	3.0053454E-5f;
    a[79] =	-4.0164417E-5f;
    a[80] =	-5.6314686E-5f;
    a[81] =	-4.0276995E-5f;
    a[82] =	-1.7250879E-5f;

FILE *iqfile=NULL;
 	iqfile=fopen("/dev/stdin","rb");
        buffer_i16 =(short*)malloc(48000*sizeof(short)*2);
FILE *outfile=NULL;
        outfile = fopen("/dev/stdout","w");
        buffer_o16 =(short*)malloc(48000*sizeof(short)*2);

        d = new Decimator(a, 83, decimation_factor);

//*---------------- Main execution loop

        (TRACE >= 0x00 ? fprintf(stderr,"%s:main(): Starting main loop\n",PROGRAMID) : _NOP); 
        setWord(&MSW,RUN,true);

	while(getWord(MSW,RUN)==true)
	{
           int nbread=fread(buffer_i16,sizeof(short),1024,iqfile);

	   if(nbread>0) {
	      //numSamplesLow=usb->generate(buffer_i16,nbread,Ibuffer,Qbuffer);
              d->decimate(buffer_i16,nbread,buffer_o16);                      //Input is fs=48KHz decimate by 8
          int numSamplesLow = nbread / decimation_factor;     
              fwrite(buffer_o16,sizeof(short),numSamplesLow,outfile);
           } else {
             setWord(&MSW,RUN,false);
           }
	}
//*---------------- program finalization cleanup
        delete(d);
        exit(0);
}

