//--------------------------------------------------------------------------------------------------
// demo_craddle controllerr   (HEADER CLASS)
// wrapper to call a different process and evaluate stability as a background activity
// This is part of the OrangeThunder platform
//--------------------------------------------------------------------------------------------------
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------
/* 
    Copyright (C) 2020 Pedro Colla (LU7DID) lu7did@gmail.com
    Copyright (C) 2015-2018  Evariste COURJAUD F5OEO (evaristec@gmail.com)
    Transmitting on HF band is surely not permitted without license (Hamradio for example).
    Usage of this software is not the responsability of the author.
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
   Thanks to first test of RF with Pifm by Oliver Mattos and Oskar Weigl 	
   INSPIRED BY THE IMPLEMENTATION OF PIFMDMA by Richard Hirst <richardghirst@gmail.com>  December 2012
   Helped by a code fragment by PE1NNZ (http://pe1nnz.nl.eu.org/2013/05/direct-ssb-generation-on-pll.html)
 */
#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 

#include <iostream>
#include <fstream>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "/home/pi/librpitx/src/librpitx.h"
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstring>
#include "/home/pi/OrangeThunder/src/OT/OT.h"




//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="demo_craddle";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

byte    MSW=0x00;


//#ifndef gpioWrapper_h
//#define gpioWrapper_h

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACKPIN)(int pin,int state);


byte       TRACE=0x02;
pid_t      pid = 0;
int        status;
int        inpipefd[2];
int        outpipefd[2];
char*      buffer;
int        len=1024;
struct     sigaction sigact;
byte       bSignal=0x00;

pthread_t  t=(pthread_t)-1;
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
// sighandler
// ======================================================================================================================
static void sighandler(int signum)
{
        fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum);
        setWord(&MSW,RUN,false);
        if (bSignal>0x00) {
           fprintf(stderr, "\n%s:sighandler() Signal terminate re-entrancy, exiting!\n",PROGRAMID);
           exit(16);
        }
        bSignal++;
        pthread_cancel(t);
}

//---------------------------------------------------------------------------------------------------
// gpio CLASS
//---------------------------------------------------------------------------------------------------
void* startProcess (void * null) {


    (TRACE>=0x02 ? fprintf(stderr,"%s::startProcess() starting thread\n",PROGRAMID) : _NOP);
    char cmd[1024];
    char buf[2048];


    sprintf(cmd,"%s","python /home/pi/OrangeThunder/bash/turnon.py && sudo pift8 -m \"CQ LU7DID GF05\" -f 14074000 -s 2 -r && python /home/pi/OrangeThunder/bash/turnoff.py");
    (TRACE>=0x02 ? printf("%s::startProcess() cmd(%s)\n",PROGRAMID,cmd) : _NOP);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        (TRACE>=0x02 ? printf("%s::startProcess() error opening pipe\n",PROGRAMID) : _NOP);
        pthread_exit(NULL);
        setWord(&MSW,RUN,false);
        pthread_exit(NULL);
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        printf("OUTPUT: %s", buf);

    }

    if(pclose(fp))  {
        (TRACE>=0x02 ? printf("%s::startProcess() command not found of exited with error status\n",PROGRAMID) : _NOP);
        setWord(&MSW,RUN,false);
        pthread_exit(NULL);
    }


    (TRACE>=0x02 ? printf("%s::startProcess() thread normally terminated\n",PROGRAMID) : _NOP);
    
    setWord(&MSW,RUN,false);
    pthread_exit(NULL);
}
// ======================================================================================================================
//                                                 MAIN
// ======================================================================================================================
int main(int argc, char* argv[])
{

  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);


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






  buffer=(char*) malloc(4096);
  setWord(&MSW,RUN,true);

int rct=pthread_create(&t,NULL, &startProcess, NULL);
  if (rct != 0) {
     fprintf(stderr,"%s:main() thread can not be created at this point rc(%d)\n",PROGRAMID,rct);
     exit(16);
  }
  fprintf(stderr,"%s:main() thread created rc(%d)\n",PROGRAMID,rct);

  //startProcess();

// --- spawn child process
  

  fprintf(stderr,"\n");

  while(getWord(MSW,RUN)==true)
  {
     fprintf(stderr,"%s:main(): main thread waiting....\n",PROGRAMID);
     sleep(1);
  }
  pthread_join(t, NULL);

}
