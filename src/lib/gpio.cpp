/*
   gpio.cpp

*/ 
/*
 * gpio 
 *-----------------------------------------------------------------------------
 * component to manage the gpio ports for the OrangeThunder platform
 * this program is neede as an external component in order to avoid a 
 * privileges circular situation where some of the components of OrangeThunder
 * (aka RF components or gpio handling libraries) requires sudo privileges while
 * other refuses to run under them (aka some sound libraries). Therefore the
 * components requiring sudo are run as external programs (child) with both 
 * std input and std output redirected in order to operate both as a stand alone
 * implementation or under a IPC scheme.
 * Copyright (C) 2020 by Pedro Colla <lu7did@gmail.com>
 * ----------------------------------------------------------------------------
 * uses the pigpio Library (e-mail: pigpio@abyz.me.uk)
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


#include <pigpio.h>
#include "/home/pi/OrangeThunder/src/OT/OT.h"

typedef unsigned char byte;
typedef bool boolean;

struct gpio_state
{
        bool     active;
        bool     mode;
        bool     pullup;
        bool     longpush;
        bool     BMULTI;
        bool     KDOWN;
struct  timeval  start;
struct  timeval  end;
};

auto     startPush=std::chrono::system_clock::now();
auto     endPush=std::chrono::system_clock::now();


const char   *PROGRAMID="gpio";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

byte   TRACE=0x00;
byte   MSW=0x00;
int    cmd_FD=UNDEFINED;
boolean q=false;
struct sigaction sigact;
char*  cmd_buffer;
struct gpio_state gpio[MAXGPIO];

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
// ======================================================================================================================
// pigpio SIGTERM handler
// ======================================================================================================================
int gpioSIGTERM() {

    setWord(&MSW,RUN,false);
    (TRACE >=0x00 ? fprintf(stderr, "\n%s:gpioSignalHandler() SIGTERM caught(%d), exiting!\n",PROGRAMID) : _NOP);
    return 0;

}
// ======================================================================================================================
// sighandler
// ======================================================================================================================
static void sighandler(int signum)
{
    (TRACE >= 0x00 ? fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum) : _NOP);
    setWord(&MSW,RUN,false);  
}
// ======================================================================================================================
// print_usage
// ======================================================================================================================
void print_usage(void)
{
	fprintf(stderr,
		"gpio, a simple gpio management program part of the OrangeThunder platform\n\n"
		"Use:\tgpio -p num:IO:pull [-v] [-t tracelevel]\n"
		"\t-q quit after processing command\n"
		"\t-p port definition (multiple allowed)\n"
		"\t    num port number [00..64]\n"
		"\t    IO  port direction I=input O=output\n"
		"\t    pullup pullup 0=no 1=yes\n"
		"\t-v  verbose [0..3]\n");
        exit(1);
}
// ======================================================================================================================
// parse port
// parse and validate port informationN
// ======================================================================================================================
int parseport(char* arg, gpio_state* gpio) {

char* e;
char* m;
char* r;
char* l;
int   index;
char  port[8];
char  mode[8];
char  pull[8];
char  push[8];
int   pin;
int   p;


    e = strchr(arg, ':');
    if (e==NULL) {
       pin=atoi(arg);
       if (pin<=0 || pin >32) {
          (TRACE>=0x02 ? fprintf(stderr,"%s:parseport invalid pin number(%s) entry ignored\n",PROGRAMID,arg) : _NOP);
          return -1;
       }
       gpio[pin].active=true;
       gpio[pin].mode=false;
       gpio[pin].pullup=false;
       gpio[pin].longpush=false;
       (TRACE >= 0x02 ? fprintf(stderr,"%s:parseport parsed PIN[%d] only pin informed assumed MODE[%s] PULLUP[%s] longPush(%s)\n",PROGRAMID,pin,BOOL2CHAR(gpio[pin].mode),BOOL2CHAR(gpio[pin].pullup),BOOL2CHAR(gpio[pin].longpush)) : _NOP);
       return pin;
    }

// --- now parse the pin our of the string

    index = (int)(e - arg);
    strncpy(port,arg,index);
    port[index]=0x00;
    pin=atoi(port);
    if (pin<=0 || pin > 32) {
          (TRACE>=0x02 ? fprintf(stderr,"%s:parseport invalid pin number(%s) entry ignored\n",PROGRAMID,arg) : _NOP);
          return -1;
    }
    e++;


// --- now parse the pin mode 

    m = strchr(e,':');
    if(m==NULL) {
      p=atoi(e);
      (p!=1 ? gpio[pin].mode=false : gpio[pin].mode=true);
      gpio[pin].pullup=false;
      gpio[pin].longpush=false;
      (TRACE >= 0x02 ? fprintf(stderr,"%s:parseport parsed PIN[%d] MODE[%s] assumed PULLUP[%s] longPush[%s]\n",PROGRAMID,pin,BOOL2CHAR(gpio[pin].mode),BOOL2CHAR(gpio[pin].pullup),BOOL2CHAR(gpio[pin].longpush)) : _NOP);
      return pin;
    }
    index = (int)(m - e);
    strncpy(mode,e,index);
    mode[index]=0x00;
    p=atoi(mode);
    (p!=1 ? gpio[pin].mode=false : gpio[pin].mode=true);
    m++;

// --- now parse the pin mode 

    r = strchr(m,':');
    if(r==NULL) {
      p=atoi(m);
      (p!=1 ? gpio[pin].pullup=false : gpio[pin].pullup=true);
      gpio[pin].longpush=false;
      (TRACE >= 0x02 ? fprintf(stderr,"%s:parseport parsed PIN[%d] MODE[%s] PULLUP[%s] assumed longPush[%s]\n",PROGRAMID,pin,BOOL2CHAR(gpio[pin].mode),BOOL2CHAR(gpio[pin].pullup),BOOL2CHAR(gpio[pin].longpush)) : _NOP);
      return pin;
    }


    index = (int)(r - m);
    strncpy(pull,m,index);
    pull[index]=0x00;
    p=atoi(pull);
    (p!=1 ? gpio[pin].pullup=false : gpio[pin].pullup=true);
    r++;

// --- now parse the pin pullup mode


    l = strchr(r,':');
    if (l!=NULL) {
       index = (int)(l - r);
    } else {
       index = strlen(r);
    }
    strncpy(push,r,index);
    push[index]=0x00;
    p=atoi(push);
    (p!=1 ? gpio[pin].longpush=false : gpio[pin].longpush=true);

    (TRACE >= 0x02 ? fprintf(stderr,"%s:parseport parsed PIN(%d) MODE(%s) PULL(%s) pushButton(%s)\n",PROGRAMID,pin,BOOL2CHAR(gpio[pin].mode),BOOL2CHAR(gpio[pin].pullup),BOOL2CHAR(gpio[pin].longpush)) : _NOP);
    return pin;
}

// ======================================================================================================================
// alert function
// this is a program callback to handle changes on the informed pin
// ======================================================================================================================
void handleGPIOAlert(int g, int level, uint32_t tick)
{

long mtime, seconds, useconds;
int  k=0;

   (TRACE>=0x03 ? fprintf(stderr,"%s:handleGPIOAlert() ISR Handler pin number(%d) level(%d) tick(%ld)\n",PROGRAMID,g,level,(long int)tick) : _NOP);

   if (g<=0 || g>=32) {
      (TRACE>=0x02 ? fprintf(stderr,"%s:handleGPIOAlert() invalid pin number(%d), ignored!\n",PROGRAMID,g) : _NOP);
      return;
   }

   if (gpio[g].active!=true) {
      (TRACE>=0x02 ? fprintf(stderr,"%s:handleGPIOAlert() pin number(%d) not active, ignored!\n",PROGRAMID,g) : _NOP);
      return;
   }

   if (level == 0) {

      if (gpio[g].BMULTI==false) {
         (TRACE>=0x02 ? fprintf(stderr,"%s:handleGPIOAlert() pin number(%d) timer not running, ignored!\n",PROGRAMID,g) : _NOP);
         return;
      }

      gettimeofday(&gpio[g].end, NULL);

      seconds  = gpio[g].end.tv_sec  - gpio[g].start.tv_sec;
      useconds = gpio[g].end.tv_usec - gpio[g].start.tv_usec;
      mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
      int lapPush=mtime;    //std::chrono::duration_cast<std::chrono::milliseconds>(endPush - gpio[g].start).count();
      (TRACE >= 0x02 ? fprintf(stderr,"%s:handleGPIOAlert() >>> Button(%d) released. Stop of timer (%d) millisecs!\n",PROGRAMID,g,lapPush) : _NOP);

      if (lapPush < MINSWPUSH) {
         (TRACE>=0x02 ? fprintf(stderr,"%s:handleGPIOALert() Push pulse too short! (%d) millisecs ignored!\n",PROGRAMID,lapPush) : _NOP);
         gpio[g].BMULTI=false;
         return;
      } else {
         //gpio[g].BMULTI=true;
         if (mtime > MAXSWPUSH) {
            (TRACE >= 0x02 ? fprintf(stderr,"%s:handleGPIOAlert() Pin(%d) pushed long t(%d) ms.\n",PROGRAMID,mtime) : _NOP);
            gpio[g].KDOWN=true;
         } else {
            (TRACE >= 0x02 ? fprintf(stderr,"%s:handleGPIOAlert() Pin(%d) pushed short t(%d) ms\n",PROGRAMID,mtime) : _NOP);
            gpio[g].KDOWN=false;
         }
         if (gpio[g].KDOWN==true && gpio[g].longpush==true) {k=2;} else {k=1;}
         fprintf(stderr,"GPIO%d=%1d\n", g, k);
         gpio[g].BMULTI=false;
         gpio[g].KDOWN=false;
         return;
       }
  }
  if (gpio[g].BMULTI==true) {
     (TRACE >= 0x03 ? fprintf(stderr,"%s:handleGPIOAlert() Received request to start while processing previouspin(%d). Ignored!\n",PROGRAMID,g) : _NOP);
     return;
  }

  gettimeofday(&gpio[g].start, NULL);
  gpio[g].BMULTI=true;
  (TRACE >= 0x02 ? fprintf(stderr,"%s:handleGPIOAlert() >>> Button(%d) pressed. Start of timer!\n",PROGRAMID,g) : _NOP);
  k=0;
  fprintf(stderr,"GPIO%d=%1d\n", g, k);

}

// call aFunction whenever GPIO 4 changes state

// ======================================================================================================================
//                                                MAIN
// ======================================================================================================================
int main(int argc, char *argv[])
{

double start;
int    anyargs=1;
char*  iGPIO;
char*  jGPIO;
char   PIN[8];
int    j=0;


  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);

// *--------------------------------------------------------------------------------
// * Initialize gpio handling structures
// *--------------------------------------------------------------------------------
   for (int i=0;i<MAXGPIO;i++) {
     gpio[i].active = false;
     gpio[i].mode   = false;
     gpio[i].pullup = false;
     gpio[i].BMULTI = false;
     gpio[i].KDOWN  = false;
     gpio[i].longpush=false;
  
   }

// *--------------------------------------------------------------------------------
// * parse arguments
// *--------------------------------------------------------------------------------


        while ( 1 ) {
             int opt = getopt(argc, argv, "p:v:hq");
             if(opt == -1) {
   	       if(anyargs) break;
	       else opt='h'; //print usage and exit
             }
	     switch (opt) {
		case 'p': {
                        int p=parseport(optarg,&gpio[0]);
                        if (p==-1) {
 			  (TRACE >= 0x01 ? fprintf(stderr,"%s  port(?) invalid, ignored!\n",PROGRAMID) : _NOP);
                        } else {
                          gpio[p].active=true;
                          (TRACE >= 0x01 ? fprintf(stderr,"%s  port=[%d,%s,%s] set\n",PROGRAMID,p,BOOL2CHAR(gpio[p].mode),BOOL2CHAR(gpio[p].pullup)) : _NOP);
                        }
			break; }
		case 'q': {
			q = true;
                        (TRACE >= 0x01 ? fprintf(stderr,"%s  QUIT=%s\n",PROGRAMID,BOOL2CHAR(q)) : _NOP);
			break; }
		case 'v': {
			TRACE = (byte)(atoi(optarg));
                        (TRACE >= 0x01 ? fprintf(stderr,"%s  TRACE=%d\n",PROGRAMID,TRACE) : _NOP);
			break; }
		case 'h':
		default: {
			print_usage();
			break; }
	     }
	}

  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

// --- Define the rest of the signal handlers, basically as termination exceptions

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

// *--------------------------------------------------------------------------------
// * Initialize the gpio library
// *--------------------------------------------------------------------------------
   if (gpioInitialise() < 0)
   {
      (TRACE >= 0x00 ? fprintf(stderr, "%s:main() pigpio initialisation failed\n",PROGRAMID) : _NOP);
      exit(16);
   }

   int rc=gpioSetSignalFunc(SIGTERM,(gpioSignalFunc_t)gpioSIGTERM);
   rc=gpioSetSignalFunc(SIGINT,(gpioSignalFunc_t)gpioSIGTERM);


   int ngpio=0;

   for (int i=0;i<MAXGPIO;i++) {
      if (gpio[i].active == true) {
         (gpio[i].mode == false ? gpioSetMode(i,PI_INPUT) : gpioSetMode(i,PI_OUTPUT));
         (gpio[i].pullup == false ? gpioSetPullUpDown(i,PI_PUD_DOWN) : gpioSetPullUpDown(i,PI_PUD_UP));
         if (gpio[i].mode==false) {
            gpioSetISRFunc(i,EITHER_EDGE,-1,handleGPIOAlert);
            (TRACE>=0x02 ? fprintf(stderr,"%s:main() gpio pin(%d) configured MODE[%s] PULL[%s] handleGPIOAlert()\n",PROGRAMID,i,BOOL2CHAR(gpio[i].mode),BOOL2CHAR(gpio[i].pullup)) : _NOP);
         } else {
            (TRACE>=0x02 ? fprintf(stderr,"%s:main() gpio pin(%d) configured MODE[%s] PULL[%s]\n",PROGRAMID,i,BOOL2CHAR(gpio[i].mode),BOOL2CHAR(gpio[i].pullup)) : _NOP);
         }

         ngpio++;
      }
   }

   if (ngpio==0) {
      (TRACE >= 0x00 ? fprintf(stderr,"%s:main() no gpio pin has been informed as active, terminating!\n",PROGRAMID) : _NOP);
      exit(16);
   } else {
      (TRACE >= 0x02 ? fprintf(stderr,"%s:main() %d gpio pin informed and activated.\n",PROGRAMID,ngpio) : _NOP);
   }
   cmd_buffer=(char*)malloc(1024);
   //cmd_FD=fopen("/dev/stdin","rb");
   cmd_FD = open ("/dev/stdin", ( O_RDONLY | O_NONBLOCK ) );
   start = time_time();
   setWord(&MSW,RUN,true);

// *--------------------------------------------------------------------------------
// *                       Main Loop                                               *
// *--------------------------------------------------------------------------------

   while (getWord(MSW,RUN) == true)
   {
      int cmd_length=read(cmd_FD,(void*)cmd_buffer,255);
          cmd_buffer[cmd_length] = 0x00;
          if(cmd_length>0) {
            (TRACE>=0x02 ? fprintf (stderr,"%s:main() Received data from command pipe (%s) len(%d)\n",PROGRAMID,cmd_buffer,cmd_length) : _NOP);
            if (strncmp(cmd_buffer,"GPIO",4)==0) {
               (TRACE>=0x02 ? fprintf (stderr,"%s:main() Received data matches GPIO\n",PROGRAMID) : _NOP);
               iGPIO=strchr(cmd_buffer,'O');
               if(iGPIO!=NULL) {
                 iGPIO++;
                 jGPIO=strchr(iGPIO,'=');
                 if (jGPIO!=NULL) {
                    j=(int)(jGPIO-iGPIO);
                    strncpy(PIN,iGPIO,j);
                    PIN[j]=0x00;
                    int pin=atoi(PIN);
                    (TRACE>=0x02 ? fprintf (stderr,"%s:main() parsed GPIO Pin(%d)\n",PROGRAMID,pin) : _NOP);
                    jGPIO++;
                    int val=atoi(jGPIO);
                    if (val==0 || val==1) {
                       (TRACE>=0x02 ? fprintf (stderr,"%s:main() parsed GPIO Pin(%d) value(%d)\n",PROGRAMID,pin,val) : _NOP);
                       if (gpio[pin].active==true) {
                          gpioWrite(pin,val);
                       }
                    }
                 }
               }

            }
          }
      usleep(100000);
      if (q==true) {
         (TRACE>=0x00 ? fprintf(stderr,"%s:main() one shot termination\n",PROGRAMID) : _NOP);
         break;
      }
   }

   /* Stop DMA, release resources */
   gpioTerminate();
   (TRACE>=0x00 ? fprintf(stderr,"%s:main() termination completed successfully\n",PROGRAMID) : _NOP);
   return 0;
}
