/*
 * genSSB.cpp
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
w * along with this program; if not, write to the Free Software
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
#include <sys/shm.h>            //Used for shared memory

#include "SSB.h"
#include "/home/pi/librpitx/src/librpitx.h"
#include "../OT/OT.h"
#include "../OT4D/transceiver.h"
#include "../lib/CallBackTimer.h"

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genSSB";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


typedef unsigned char byte;
typedef bool boolean;

//----- SHARED MEMORY -----
struct shared_memory_struct {
        int  ptt_flag;
        bool ptt_signal;
        int  vox_flag;
        bool vox_signal;
        char common_data[1024];
};


char*    cmd_buffer;
int      cmd_length;
int      cmd_FD = UNDEFINED;
int      cmd_result;

int      ax;

byte     MSW=0;
byte     TRACE=0x00;

byte     fVOX=0;
long int TVOX=0;
bool     fquiet=false;
bool     fdds=false;

FILE *iqfile=NULL;
FILE *outfile=NULL;

float    agc_reference=AGC_REF;
float    agc_max=AGC_MAX;
float    agc_alpha=AGC_ALPHA;
float    agc_thr=agc_max*AGC_LEVEL;
float    agc_gain=AGC_GAIN;;

//int      vox_timeout=VOX_TIMEOUT;  //default=2 secs
int      vox_timeout=0;  //default=2 secs
int      gpio_ptt=GPIO_PTT;

short    *buffer_i16;
SSB*     usb;
float*   Ibuffer;
float*   Qbuffer;
float*   Fout;
bool     autoPTT=false;
int      nbread=0;
int      numSamplesLow=0;
int      result=0;
char     timestr[16];
CallBackTimer* VOXtimer;

void    *pshared = (void *)0;
//VARIABLES:

struct   shared_memory_struct *sharedmem=nullptr;
int      sharedmem_id=0;
int      sharedmem_token=0;

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
// returns the time in a string format
//--------------------------------------------------------------------------------------------------
char* getTime() {

       time_t theTime = time(NULL);
       struct tm *aTime = localtime(&theTime);
       int hour=aTime->tm_hour;
       int min=aTime->tm_min;
       int sec=aTime->tm_sec;
       sprintf(timestr,"%02d:%02d:%02d",hour,min,sec);
       return (char*) &timestr;

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
       fVOX=1;
       (TRACE>=0x02 ? fprintf(stderr,"%s:timer_exec %s Timer TVOX expired\n",PROGRAMID,getTime()) : _NOP);
     }
  }
}
//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\
Usage:  \n\
-p            PTT GPIO port (default none)\n\
-a            agc threshold level (default none)\n\
-d            enable DDS operation (default none)\n\
-x            enable auto PTT on VOX (default none)\n\
-m            enable shared memory commands (default no)\n\
-q            quiet operation (default none)\n\
-v            VOX activated timeout in secs (default 0 )\n\
-t            DEbug level (0=no debug, 2=max debug)\n\
-?            help (this help).\n\
\n",PROGRAMID,PROG_VERSION,PROG_BUILD);


} /* end function print_usage */

//--------------------------------------------------------------------------------------------------
// FT8ISR - interrupt service routine, keep track of the FT8 sequence windows
//--------------------------------------------------------------------------------------------------
void VOXISR() {

     timer_exec();
}
//--------------------------------------------------------------------------------------------------
// writePin
//--------------------------------------------------------------------------------------------------
void writePin(int pin, int v) {

     gpioWrite(pin,v);
    (TRACE>=0x02 ? fprintf(stderr,"%s:writePin executed pin(%d) valud(%d)\n",PROGRAMID,pin,v) : _NOP);

}

//---------------------------------------------------------------------------------
// setPTT
//---------------------------------------------------------------------------------
void setPTT(bool ptt) {

   (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT() set PTT(%s)\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);

   if (ptt==true) {
     if (getWord(MSW,PTT)==false) {                // Signal RF generator now is I/Q mode
        writePin(gpio_ptt,1);
        (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT() sending 1111 to sendRF\n",PROGRAMID) : _NOP);
        Fout[0]=1111.0;
        Fout[1]=1111.0;
        fwrite(Fout, sizeof(float), 2, outfile) ;
        usleep(100);
      }
      setWord(&MSW,PTT,true); //Signal PTT as ON
      return;
   }

   writePin(gpio_ptt,0);
  (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT() PTT turned off\n",PROGRAMID) : _NOP);

   if (getWord(MSW,PTT)==true && fdds==true) { //Signal RF Generator the mode is now FREQ_A (only if dds mode allowed)
      (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT() sending 2222 to sendRF\n",PROGRAMID) : _NOP);
      Fout[0]=2222.0;
      Fout[1]=2222.0;
      fwrite(Fout, sizeof(float), 2, outfile) ;
      usleep(100);
   }
   setWord(&MSW,PTT,false); // Signal the PTT as OFF

}
//---------------------------------------------------------------------------------
// terminate
//---------------------------------------------------------------------------------
static void terminate(int num)
{
    setWord(&MSW,RUN,false);
    (TRACE>=0x00 ? fprintf(stderr,"%s: Caught TERM signal(%x) - Terminating \n",PROGRAMID,num) : _NOP);
    if (getWord(MSW,RETRY)==true) {
       (TRACE>=0x00 ? fprintf(stderr,"%s: Signal handler re-entrancy, force exit SIG(%x) - Terminating \n",PROGRAMID,num) : _NOP);
       exit(16);
    }
    setWord(&MSW,RETRY,true);
}
//---------------------------------------------------------------------------------
// main 
//---------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

	while(1)
	{
		ax = getopt(argc, argv, "a:v:m:p:dxqt:");
                if(ax == -1) 
		{
			if(ax) break;
			else ax='h'; //print usage and exit
		}

	
		switch(ax)
		{
		case 'a': // AGC Maxlevel
			agc_max = atof(optarg);
                        if (agc_max>0.0) {
	                   agc_thr=agc_max*AGC_LEVEL;
	  		   (TRACE >= 0x01 ? fprintf(stderr,"%s:Args() AGC max level(%2f) threshold(%2f)\n",PROGRAMID,agc_max,agc_thr) : _NOP);
  	                } else {
	  	           agc_max=0.0;
		 	   agc_thr=0.0;
	  		   (TRACE >= 0x00 ? fprintf(stderr,"%s:Args() AGC max invalid\n",PROGRAMID) : _NOP);
		        }
			break;
                case 't': // Debug level
                        TRACE = atoi(optarg);
                        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- Debug level established TRACE(%d)\n",PROGRAMID,TRACE) : _NOP);
                        break;
                case 'm': // shared memory token
                        sharedmem_token = atoi(optarg);
                        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- Shared memory command area token(%d)\n",PROGRAMID,sharedmem_token) : _NOP);
                        break;
		case 'v': // VOX Timeout
			vox_timeout = atoi(optarg);
   		        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- VOX enabled timeout(%d) Secs\n",PROGRAMID,vox_timeout) : _NOP);
			break;
		case 'q': // go quiet
			fquiet=true;
  		        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- Quiet operation (no messages)\n",PROGRAMID) : _NOP);
			break;
		case 'x': // go quiet
			autoPTT=true;
  		        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- Auto PTT set\n",PROGRAMID) : _NOP);
			break;
		case 'd': // DDS mode
			fdds=true;
  		        (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- DDS operation enabled\n",PROGRAMID) : _NOP);
			break;
		case 'p': //GPIO PTT
			gpio_ptt  = atoi(optarg);
		        if (gpio_ptt > 0) {
   		           (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- PTT enabled GPIO%d\n",PROGRAMID,gpio_ptt) : _NOP);
		        } else {
	    		   (TRACE>=0x01 ? fprintf(stderr,"%s:main() args--- invalid PTT GPIO pin\n",PROGRAMID) : _NOP);
                        }
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) ) {
 			   fprintf(stderr, "%s:main() args---  unknown option `-%c'.\n",PROGRAMID,optopt);
 			} else 	{
		           fprintf(stderr, "%s:main() args---  unknown option character `\\x%x'.\n",PROGRAMID,optopt);
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


//*---------------------------------------------------------------------------------------------------------
//* Follows a setup valid when the -d and -x conditions are set, which is the pase of Pi4D
//*---------------------------------------------------------------------------------------------------------

   fprintf(stderr,"%s %s [%s] tracelevel(%d) dds(%s) PTT(%s)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,TRACE,BOOL2CHAR(fdds),BOOL2CHAR(autoPTT));

        (TRACE>=0x02 ? fprintf(stderr,"%s:main(): Trap handler initialization\n",PROGRAMID) : _NOP);
	for (int i = 0; i < 64; i++) {
           if (i != SIGALRM && i != 17 && i != 28) {
               struct sigaction sa;
               std::memset(&sa, 0, sizeof(sa));
               sa.sa_handler = terminate;
               sigaction(i, &sa, NULL);
           }

        }



//--------------------------------
//----- CREATE SHARED MEMORY -----
//--------------------------------
//      Shared memory key
//              Unique non zero integer (usually 32 bit).  Needs to avoid clashing with another other processes shared memory (you just have to pick a random value and hope - ftok() can help$
//      Permission flags
//              Operation permissions   Octal value
//              Read by user                    00400
//              Write by user                   00200
//              Read by group                   00040
//              Write by group                  00020
//              Read by others                  00004
//              Write by others                 00002
//              Examples:
//                      0666 Everyone can read and write


   if (sharedmem_token != 0) {
      (TRACE>=0x02 ? fprintf(stderr,"%s:genSSB() Creating shared memory...\n",PROGRAMID) : _NOP);
      sharedmem_id = shmget((key_t)sharedmem_token, sizeof(struct shared_memory_struct), 0666 | IPC_CREAT);           //<<<<< SET THE SHARED MEMORY KEY    (Shared memory key , Size in bytes, Permission fl$

      if (sharedmem_id == -1) {
         (TRACE>=0x00 ? fprintf(stderr, "%s:genSSB() Shared memory shmget() failed\n",PROGRAMID) : _NOP);
          exit(16);
      }

//Make the shared memory accessible to the program

      pshared = shmat(sharedmem_id, (void *)0, 0);
      if (pshared == (void *)-1)   {
         (TRACE>=0x00 ? fprintf(stderr, "%s:genSSB() Shared memory shmat() failed\n",PROGRAMID) : _NOP);
         exit(16);
      }

      (TRACE>=0x00 ? fprintf(stderr,"%s:main() Shared memory attached at %X\n",PROGRAMID,(int)pshared) : _NOP);

//Assign the shared_memory segment

      sharedmem = (struct shared_memory_struct *)pshared;

//----- READ FROM SHARED MEMORY -----

     (TRACE>=0x00 ? fprintf(stderr,"%s::start() shared memory initialized ptt(%d/%s) vox(%d/%s)\n",PROGRAMID,sharedmem->ptt_flag,BOOL2CHAR(sharedmem->ptt_signal),sharedmem->vox_flag,BOOL2CHAR(sharedmem->vox_signal)) : _NOP);
   }

//*---- this is enabled only for the Orange Thunder platform which is the only that requires genSSB to keep the DDS turned on at all times

    gpioCfgClock(5, 0, 0);
    if (gpioInitialise()<0) {
       (TRACE>=0x00 ? fprintf(stderr,"%s:main() Cannot initialize GPIO sub-system\n",PROGRAMID) : _NOP);
        exit(16);
    } else {
      (TRACE>=0x00 ? fprintf(stderr,"%s:main() GPIO sub-system initialized\n",PROGRAMID) : _NOP);
    }

   (TRACE>=0x02 ? fprintf(stderr,"%s:main() Setup PTT\n",PROGRAMID) : _NOP);
    gpioSetMode(gpio_ptt, PI_OUTPUT);
    gpioSetPullUpDown(gpio_ptt, PI_PUD_UP);   // Sets a pull-up.
    gpioWrite(gpio_ptt, 0);

//*--- Again the fdds==true is used as an indication of the PixiePi platform, which is the one having a cooler (OT doesn't have one)

    if (fdds==true) {
       (TRACE>=0x02 ? fprintf(stderr,"%s:main() Setup Cooler\n",PROGRAMID) : _NOP);
        gpioSetMode(GPIO_COOLER, PI_OUTPUT);
        gpioSetPullUpDown(GPIO_COOLER, PI_PUD_UP);   // Sets a pull-up.
        gpioWrite(GPIO_COOLER, 1);
    }

    for (int i=0;i<64;i++) {
         gpioSetSignalFunc(i,terminate);
    }


//*------ end of GPIO initialization

        //vox_timeout=2;
        (TRACE>=0x02 ? fprintf(stderr,"%s:main(): VOX timeout set to (%d) msecs\n",PROGRAMID,vox_timeout) : _NOP);

 	iqfile=fopen("/dev/stdin","rb");
        outfile=fopen("/dev/stdout","wb");

        buffer_i16 =(short*)malloc(AFRATE*sizeof(short)*2);
        Ibuffer =(float*)malloc(IQBURST*sizeof(short)*2);
        Qbuffer =(float*)malloc(IQBURST*sizeof(short)*2);
        Fout    =(float*)malloc(IQBURST*sizeof(short)*4);
        cmd_buffer=(char*)malloc(1024);

        (TRACE>=0x02 ? fprintf(stderr,"%s:main(): Define master timer\n",PROGRAMID) : _NOP);       
        VOXtimer=new CallBackTimer();
        VOXtimer->start(1,VOXISR);

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
        (TRACE >= 0x02 ? fprintf(stderr,"%s:main(): SSB controller generation completed\n",PROGRAMID) : _NOP); 

  float maxgain=0.0;
  float mingain=usb->agc.max_gain;
  float thrgain=usb->agc.max_gain*0.50;

//*---------------- Main execution loop

        setWord(&MSW,PTT,false);
   int  j=0;
        (TRACE >= 0x00 ? fprintf(stderr,"%s:main(): Starting main loop VOX(%s) Timeout(%d)\n",PROGRAMID,BOOL2CHAR(autoPTT),vox_timeout) : _NOP); 

        vox_timeout=vox_timeout*1000;  //interface is expressed in secs but actual timer is in mSecs

        setWord(&MSW,RUN,true);
	while(getWord(MSW,RUN)==true)
	{

// --- process commands thru pipe

        if (sharedmem != nullptr) {
           if (sharedmem->ptt_flag!=0 ) {
              (TRACE >= 0x02 ? fprintf(stderr,"%s:main(): PTT flag marker detected autoPTT(%s)\n",PROGRAMID,BOOL2CHAR(autoPTT)) : _NOP); 
               sharedmem->ptt_flag=0;
               if (autoPTT==false) {
                   setPTT(sharedmem->ptt_signal);
                  (TRACE >= 0x02 ? fprintf(stderr,"%s:main(): PTT(%s) signal received\n",PROGRAMID,BOOL2CHAR(sharedmem->ptt_signal)) : _NOP); 
               } else {
                  (TRACE >= 0x02 ? fprintf(stderr,"%s:main(): PTT(%s) signal received but autoPTT=true, ignored!\n",PROGRAMID,BOOL2CHAR(sharedmem->ptt_signal)) : _NOP); 
               }
           }
        }
// --- end of command processing, read signal samples

        nbread=fread(buffer_i16,sizeof(short),1024,iqfile);

// --- Processing AGC results on  incoming signal

        if (gain<mingain) {
           (TRACE>=0x02 ? fprintf(stderr,"%s:main() gain(%f)<mingain(%f) corrected mingain\n",PROGRAMID,gain,mingain) : _NOP);
            mingain=gain;
        }

        if (gain>maxgain) {
           (TRACE>=0x02 ? fprintf(stderr,"%s:main() gain(%f)>maxgain(%f) corrected maxgain\n",PROGRAMID,gain,maxgain) : _NOP);
            maxgain=gain;
            thrgain=maxgain*0.70;
        }

        if (vox_timeout > 0) {  //Is the timeout activated?
            if (gain<thrgain) {  //Is the current gain lower than the thr? (lower the gain --> bigger the signal
                if (TVOX==0 && sharedmem != nullptr) {    //Is the timer counter idle
                    sharedmem->vox_signal=true;
                    sharedmem->vox_flag=1;
                    (TRACE>=0x00 ? fprintf(stderr,"%s:main() vox_signal(true) signal sent\n",PROGRAMID) : _NOP);
                 }
                 TVOX=vox_timeout; //Refresh the timeout
                 fVOX=0;
                 if (autoPTT == true && getWord(MSW,PTT)==false) { //If auto PTT enabled and currently PTT=off then make it On
                     setPTT(true);
                    (TRACE>=0x00 ? fprintf(stderr,"%s:main() autoPTT=true set PTT(true)\n",PROGRAMID) : _NOP);
                     setWord(&MSW,PTT,true);
                 }
             }

             if (fVOX==1) {  //Has the timer reach zero?
                 fVOX=0;      //Clear Mark
                 if (sharedmem!=nullptr) {
                     sharedmem->vox_signal=false;
                     sharedmem->vox_flag=1;
                    (TRACE>=0x00 ? fprintf(stderr,"%s:main() vox_signal(false) signal sent as upcall\n",PROGRAMID) : _NOP);
                 }
                 if (autoPTT==true && getWord(MSW,PTT)==true) { //If auto PTT and PTT is On then make it Off
                     setPTT(false);
                    (TRACE>=0x00 ? fprintf(stderr,"%s:main() autoPTT=false set PTT(true)\n",PROGRAMID) : _NOP);
                     setWord(&MSW,PTT,false);
                 }
             }
           }

	   if(nbread>0) { // now process the audio samples into an I/Q signal
	      numSamplesLow=usb->generate(buffer_i16,nbread,Ibuffer,Qbuffer);
              if (getWord(MSW,PTT)==true) {
                 for (int i=0;i<numSamplesLow;i++) {
                     Fout[2*i]=Ibuffer[i];
                     Fout[(2*i)+1]=Qbuffer[i];
                 }
              } else {
                 for (int i=0;i<numSamplesLow;i++) {
                     Fout[2*i]=0.0;
                     Fout[(2*i)+1]=0.0;
                 }
              }
              fwrite(Fout, sizeof(float), numSamplesLow*2, outfile) ;  //Send it
              usleep(1000);
           } else {
              setWord(&MSW,RUN,false);
 	   }
	}
//*---------------- program finalization cleanup

        delete(usb);

//--------------------------------
//----- DETACH SHARED MEMORY -----
//--------------------------------
        if (sharedmem != nullptr) {   //detach from shared memory área
           if (shmdt(pshared) == -1) {
              (TRACE>=0x00 ? fprintf(stderr, "%s:main() shmdt failed\n",PROGRAMID) : _NOP);
           } else {
             //if (shmctl(sharedmem_id, IPC_RMID, 0) == -1) {
             //   (TRACE>=0x00 ? fprintf(stderr, "%s:main() shmctl(IPC_RMID) failed\n",PROGRAMID) : _NOP);
             //}
           }
           (TRACE>=0x00 ? fprintf(stderr, "%s:main() shmctl(IPC_RMID) successfully completed\n",PROGRAMID) : _NOP);
        }

        if (fdds==true) {
            gpioWrite(GPIO_COOLER, 0);
        }

        gpioWrite(gpio_ptt, 0);

        (TRACE>=0x00 ? fprintf(stderr,"%s:main() program terminated normally\n",PROGRAMID) : _NOP);
        exit(0);

}

