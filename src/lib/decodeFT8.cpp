/*
   decodeFT8.cpp
   Simple FT8 decoding program largely based after KGOBA/ft8_lib/decode_ft8.cpp
   This program can process live signals out of a radio source
   Everything works with a 12000 samples/sec.
/*
 *-----------------------------------------------------------------------------
 * Copyright (C) 2020 by Pedro Colla <lu7did@gmail.com>
 * ----------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h> 
#include <iostream>
#include <chrono>

#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

#include <pigpio.h>
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include <functional>
#include <chrono>
#include <future>
#include <cstdio>
#include <functional>
#include <chrono>
#include <future>
#include <cstdio>
#include <time.h>
using namespace std;

#define FT8LISTEN 0B00000001
#define FT8PROC   0B00000010
#define FT8WINDOW 0B00000100

#define LISTEN    12400
#define PROCESS   2300
#define MINSAMPLE 100000
#define MAXSAMPLE 210000


typedef unsigned char byte;
typedef bool boolean;

struct FT8msg {
        int db;         // Signal SNR
      float DT;         // Time Shift
        int offset;     // Frequency offset
      char* msg;      // Message received
};


const char   *PROGRAMID="decodeFT8";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
const char   *LF="\n";

byte   TRACE=0x00;
byte   MSW=0x00;
byte   FT8=0x00;

#include "/home/pi/OrangeThunder/src/lib/libFT8.h"

FT8msg slotmsg[kMax_decoded_messages];
int    num_samples=0;


int    sample_rate=12000;

struct sigaction sigact;
char   timestr[16];

int     FT8process_counter=0;
int     FT8listen_counter=0;
int     anyargs=1;
bool    bRetry=false;

FILE    *iqfile=NULL;
short   *buffer_i16=NULL;

//--------------------------[Timer Interrupt Class]-------------------------------------------------
// Implements a timer tick class calling periodically a function 
// Function passed as the upcall() needs to be implemented
//--------------------------------------------------------------------------------------------------
class CallBackTimer
{
public:

    CallBackTimer()
    :_execute(false)
    {}

    ~CallBackTimer() {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
    }

    void stop()
    {

        _execute.store(false, std::memory_order_release);
        if( _thd.joinable() )
            _thd.join();
    }

    void start(int interval, std::function<void(void)> upcall)
    {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };

        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, upcall]()
        {
            while (_execute.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(
                std::chrono::milliseconds(interval));
                upcall();

            }
        });
    }

    bool is_running() const noexcept {
        return ( _execute.load(std::memory_order_acquire) && 
                 _thd.joinable() );
    }

private:
    std::atomic<bool> _execute;
    std::thread _thd;
};


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
// ======================================================================================================================
// sighandler
// ======================================================================================================================
static void sighandler(int signum)
{
    if (bRetry==true) {
       exit(16);
    }
    (TRACE >= 0x00 ? fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum) : _NOP);
    setWord(&MSW,RUN,false);  
    bRetry=true;
}
// ======================================================================================================================
// print_usage
// ======================================================================================================================
void print_usage(void)
{
	fprintf(stderr,"%s [-v tracelevel] [-s samplerate] [-h]\n",PROGRAMID);
        exit(1);
}

//--------------------------------------------------------------------------------------------------
// FT8ISR - interrupt service routine, keep track of the FT8 sequence windows
//--------------------------------------------------------------------------------------------------
void FT8ISR() {

   if (getWord(FT8,FT8LISTEN)==true) {
      FT8listen_counter--;
      if (FT8listen_counter==0) {
         setWord(&FT8,FT8LISTEN,false);
         (TRACE>=0x03 ? fprintf(stderr,"%s:FT8listenISR() %s Listening window  expired \n",PROGRAMID,getTime()) : _NOP);
      }
   }

  if (getWord(FT8,FT8PROC)==true) {
      FT8process_counter--;
      if (FT8process_counter==0) {
         setWord(&FT8,FT8PROC,false);
         (TRACE>=0x03 ? fprintf(stderr,"%s:FT8procISR() %s  Processing window expired \n",PROGRAMID,getTime) : _NOP);
      }
   }
}
// ======================================================================================================================
//                                                MAIN
// ======================================================================================================================
int main(int argc, char *argv[])
{
  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);

// *--------------------------------------------------------------------------------
// * parse arguments (dummy for now)
// *--------------------------------------------------------------------------------
        while ( 1 ) {
             int opt = getopt(argc, argv, "s:v:h");
             if(opt == -1) {
   	       if(anyargs) break;
	       else opt='h'; //print usage and exit
             }
	     switch (opt) {
		case 't': {
			sample_rate = (atoi(optarg));
                        (TRACE >= 0x00 ? fprintf(stderr,"%s  sample_rate=%d\n",PROGRAMID,sample_rate) : _NOP);
			break; }
		case 'v': {
			TRACE = (byte)(atoi(optarg));
                        (TRACE >= 0x00 ? fprintf(stderr,"%s  TRACE=%d\n",PROGRAMID,TRACE) : _NOP);
			break; }
		case 'h':
		default: {
			print_usage();
			break; }
	     }
	}

// --- establish signal handlers

  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  (TRACE >= 0x02 ? fprintf(stderr,"%s:main() initialize interrupt handling system\n",PROGRAMID) : _NOP);
  for (int i = 0; i < 64; i++) {
      if (i != SIGALRM && i != 17 && i != 28) {
         signal(i,sighandler);
      }
  }

  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  signal(SIGPIPE, SIG_IGN);



  iqfile=fopen("/dev/stdin","rb");

// --- Define timer and kick execution

int k=0;
int nbread=0;
  buffer_i16 =(short*)malloc(48000*sizeof(short)*2);

  CallBackTimer* FT8timer;
  FT8timer=new CallBackTimer();
  FT8timer->start(1,FT8ISR);
  setWord(&MSW,RUN,true);

int secAnt=0;
int slot;
int prevslot=0;
boolean firstRun=true;
boolean slotSkip=false;
// *--------------------------------------------------------------------------------
// *                       Main Loop                                               *
// *--------------------------------------------------------------------------------
   while (getWord(MSW,RUN) == true)
   {
       time_t theTime = time(NULL);
       struct tm *aTime = localtime(&theTime);
       int sec=aTime->tm_sec;

// --- Wait till the FT8 15 secs window happens, align frame reception with it

       setWord(&FT8,FT8LISTEN,true);

// --- Wait till the reception window terminates

       num_samples=0;
       while (getWord(FT8,FT8LISTEN)==true && getWord(MSW,RUN)==true) {
           nbread=fread(buffer_i16,sizeof(short),512,iqfile);

           if (getWord(FT8,FT8WINDOW)==false) {
              theTime = time(NULL);
              tm *aTime = localtime(&theTime);
              sec=aTime->tm_sec;
              if ((sec%15)==0 ) {

                 slot=(sec/15);

                 if ((prevslot+1)%4 != slot && firstRun == false) {
                    (TRACE>=0x02 ? fprintf(stderr,"%s:FT8<loop> **** ATTENTION current slot(%d) previous slot(%d) entire slot missed\n",PROGRAMID,(slot%4),(prevslot%4)) : _NOP);
                   slotSkip=true;
                 } else {
                   slotSkip=false;
                 }
                 prevslot=slot;
                 firstRun=false;

                 setWord(&FT8,FT8WINDOW,true);
                 FT8listen_counter=LISTEN;
                 num_samples=0;
                 (TRACE >= 0x00 ? fprintf(stderr,"%s:FT8<loop> --------[%s]------------------[slot %d - Even %d]-----[%s]----------\n",PROGRAMID,getTime(),slot,(slot%2),BOOL2CHAR(slotSkip)) : _NOP);
              }
           }

           if (getWord(FT8,FT8WINDOW)==true) {
              (TRACE>=0x03 ? fprintf(stderr,"%s:FT8<loop> read from stdin %d bytes\n",PROGRAMID,nbread) : _NOP);
              for (int i = 0; i < nbread; i++) {
                signalDSP[num_samples++] = (float) (buffer_i16[i] / 32768.0f);
              }
           }
       }

       FT8process_counter=PROCESS;
       setWord(&FT8,FT8PROC,true);
       (TRACE>= 0x02 ? fprintf(stderr,"%s:FT8<loop> %s completed FT8 listening window samples(%d)\n",PROGRAMID,getTime(),num_samples) : _NOP);
  
// --- Wail till the processing window terminates

       while (getWord(FT8,FT8PROC)==true && getWord(MSW,RUN)==true) {
           if (num_samples < MINSAMPLE) {
              (TRACE>= 0x02 ? fprintf(stderr,"%s:FT8<loop> Samples less than 100000 samples(%d)\n",PROGRAMID,num_samples) : _NOP);
              break;
           }

           (TRACE>= 0x02 ? fprintf(stderr,"%s:FT8<loop> Sent to FT8_process samples(%d)\n",PROGRAMID,num_samples) : _NOP);
           if (num_samples <= MAXSAMPLE) {
              //FT8_process(num_samples,sample_rate);
              int ndecoded=FT8_process(num_samples,sample_rate,slotmsg);
              if (ndecoded!=0) {
                 for (int i=0; i<ndecoded ; i++) {
                    fprintf(stderr,"%s %d %4.1f %d %s\n",getTime(),slotmsg[i].db,slotmsg[i].DT,slotmsg[i].offset,slotmsg[i].msg);
                    free(slotmsg[i].msg);
                 }
              }
                
           } else {
             (TRACE>= 0x02 ? fprintf(stderr,"%s:FT8<loop> Number of samples way too large, ignored samples(%d)\n",PROGRAMID,num_samples) : _NOP);
           }
           setWord(&FT8,FT8PROC,false);
       }
       setWord(&FT8,FT8WINDOW,false);
       (TRACE >= 0x02 ? fprintf(stderr,"%s:FT8<loop> %s completed FT8 processing window\n",PROGRAMID,getTime()) : _NOP);
   }

   delete(FT8timer);
   return 0;
}
