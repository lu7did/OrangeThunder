/*
 * genSSB.cpp
 * Raspberry Pi based USB experimental SSB Generator
 * Experimental version largely modelled after Generator.java by Takafumi INOUE (JI3GAB)
 *---------------------------------------------------------------------
 * This program operates as a controller for a Raspberry Pi to control
 * a Pixie transceiver hardware.
 * Project at http://www.github.com/lu7did/PixiePi
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


#define PROGRAM_VERSION "1.0"
#define PTT 0B00000001
#define VOX 0B00000010
#define RUN 0B00000100
#define MAX_SAMPLERATE 200000
#define BUFFERSIZE      96000
#define IQBURST          4000


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

#include "/home/pi/PixiePi/src/lib/SSB.h"
//#include "/home/pi/PixiePi/src/lib/Decimator.h" 
//#include "/home/pi/PixiePi/src/lib/Interpolator.h" 
//#include "/home/pi/PixiePi/src/lib/FIRFilter.h" 
//#include "/home/pi/PixiePi/src/lib/AGControl.h" 
#include "/home/pi/librpitx/src/librpitx.h"

#define AGC_LEVEL 0.80
#define IQBURST   4000
#define AFRATE   48000

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genSSB";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

typedef unsigned char byte;
typedef bool boolean;

bool     running=true;
float    SetFrequency=14074000;
float    SampleRate=6000;
char*    FileName=NULL;
int      Decimation=1;
long int TVOX=0;
int      ax;
byte     MSW=0;


float    agc_reference=1.0;
float    agc_max=5.0;
float    agc_alpha=0.25;
float    agc_thr=agc_max*AGC_LEVEL;
float    agc_gain=1.0;

int      vox_timeout=0;
int      gpio_ptt=12;

short    *buffer_i16;
SSB*     usb;
float*   Ibuffer;
float*   Qbuffer;
float*   Fout;

int      nbread=0;
int      numSamplesLow=0;

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

//--------------------------------------------------------------------------------------------------
// timer_exec 
// timer management
//--------------------------------------------------------------------------------------------------
void timer_exec()
{
  if (TVOX!=0) {
     TVOX--;
     if(TVOX==0) {
       fprintf(stderr,"VOX turned off\n");
     }
  }
}

//---------------------------------------------------------------------------
// Timer handler function
//---------------------------------------------------------------------------
void timer_start(std::function<void(void)> func, unsigned int interval)
{
  std::thread([func, interval]()
  {
    while (getWord(MSW,RUN)==true)
    {
      auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
      func();
      std::this_thread::sleep_until(x);
    }
  }).detach();
}

//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\
Usage: [-i File Input][-s Samplerate][-l] [-f Frequency] [-h Harmonic number] \n\
-s            SampleRate 10000-250000 \n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-p            PTT GPIO port (default none)\n\
-a            agc threshold level (default none)\n\
-v            VOX activated (default none)\n\
-?            help (this help).\n\
\n",PROGRAMID,PROG_VERSION,PROG_BUILD);


} /* end function print_usage */
//---------------------------------------------------------------------------------
// terminate
//---------------------------------------------------------------------------------

static void terminate(int num)
{
    running=false;
    fprintf(stderr,"%s: Caught TERM signal(%x) - Terminating \n",PROGRAMID,num);
}
//---------------------------------------------------------------------------------
// main 
//---------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

        timer_start(timer_exec,100);
        fprintf(stderr,"%s %s [%s]\n",PROGRAMID,PROG_VERSION,PROG_BUILD);

	while(1)
	{
		ax = getopt(argc, argv, "a:f:s:v:p:");
                if(ax == -1) 
		{
			if(ax) break;
			else ax='h'; //print usage and exit
		}

	
		switch(ax)
		{
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			fprintf(stderr,"%s: Frequency(%f)\n",PROGRAMID,SetFrequency);
			break;
		case 's': // SampleRate (Only needeed in IQ mode)
			SampleRate = atof(optarg);
			fprintf(stderr,"%s: SampleRate(%f)\n",PROGRAMID,SampleRate);
			break;

		case 'a': // AGC Maxlevel
			agc_max = atof(optarg);
                        if (agc_max>0.0) {
	                   agc_thr=agc_max*AGC_LEVEL;
	  		   fprintf(stderr,"%s: AGC max level(%2f) threshold(%2f)\n",PROGRAMID,agc_max,agc_thr);
  	                } else {
	  	           agc_max=0.0;
		 	   agc_thr=0.0;
	  		   fprintf(stderr,"%s: AGC max invalid\n",PROGRAMID);
		        }
			break;

		case 'v': // VOX Timeout
			vox_timeout = atoi(optarg);
		        if (vox_timeout > 0) {
   		           fprintf(stderr,"%s: VOX enabled timeout(%d) mSecs\n",PROGRAMID,vox_timeout);
		        } else {
	    		   fprintf(stderr,"%s: invalid VOX timeout\n",PROGRAMID);
                        }
			break;
		case 'p': //GPIO PTT
			gpio_ptt  = atoi(optarg);
		        if (gpio_ptt > 0) {
   		           fprintf(stderr,"%s: PTT enabled GPIO%d\n",PROGRAMID,gpio_ptt);
		        } else {
	    		   fprintf(stderr,"%s: invalid PTT GPIO pin\n",PROGRAMID);
                        }
			break;

		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 			   fprintf(stderr, "%s: unknown option `-%c'.\n",PROGRAMID,optopt);
 			}
			else
			{
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

        fprintf(stderr,"%s:main(): Trap handler initialization\n",PROGRAMID);

	for (int i = 0; i < 64; i++) {
           struct sigaction sa;
           std::memset(&sa, 0, sizeof(sa));
           sa.sa_handler = terminate;
           sigaction(i, &sa, NULL);
        }

FILE *iqfile=NULL;
FILE *outfile=NULL;

 	iqfile=fopen("/dev/stdin","rb");
        outfile=fopen("/dev/stdout","wb");
        fprintf(stderr,"%s Sound data from Standard Input\n",PROGRAMID);

        buffer_i16 =(short*)malloc(AFRATE*sizeof(short)*2);
        Ibuffer =(float*)malloc(IQBURST*sizeof(short)*2);
        Qbuffer =(float*)malloc(IQBURST*sizeof(short)*2);
        Fout    =(float*)malloc(IQBURST*sizeof(short)*4);

float   gain=1.0;

        usb=new SSB();
        usb->agc.reference=agc_reference;
	usb->agc.max_gain=agc_max;
	usb->agc.rate=agc_alpha;
	usb->agc.gain=&gain;

        if (agc_max>0) {
           usb->agc.active=true;
        } else {
           usb->agc.active=false;
        }
        fprintf(stderr,"%s:main(): SSB controller generation completed\n",PROGRAMID); 


        fprintf(stderr,"%s:main(): Program execution starting\n",PROGRAMID);

    int k=0;
//*---------------- executin loop
	while(running==true)
	{
           nbread=fread(buffer_i16,sizeof(short),1024,iqfile);
           k=k+nbread;;
           if(k%10000 == 0) {
             fprintf(stderr,"%s Buffer read (%d) gain(%g)\n",PROGRAMID,k,gain);
           }
	   if(nbread>0) {
	      numSamplesLow=usb->generate(buffer_i16,nbread,Ibuffer,Qbuffer);
              for (int i=0;i<numSamplesLow;i++) {
                  Fout[2*i]=Ibuffer[i];
                  Fout[(2*i)+1]=Qbuffer[i];
              }
              fwrite(Fout, sizeof(float), numSamplesLow*2, outfile);
           } else {
   	      fprintf(stderr,"%s: End of file\n",PROGRAMID);
              running=false;
 	   }
	}
//*---------------- program finalization cleanup
        delete(usb);
 	fprintf(stderr,"%s: USB generation object terminated\n",PROGRAMID);

}

