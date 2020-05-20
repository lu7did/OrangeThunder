//--------------------------------------------------------------------------------------------------
// gpio controllerr   (HEADER CLASS)
// wrapper to call gpio from an object managing different extended functionalities
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef gpioWrapper_h
#define gpioWrapper_h

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

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACKPIN)(int pin,int state);
typedef void (*CALLBACKENC)(int clk,int dt,int state);

bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);

struct gpio_state
{
        bool     active;
        bool     mode;
        bool     pullup;
        bool     longpush;
};
//---------------------------------------------------------------------------------------------------
// gpio CLASS
//---------------------------------------------------------------------------------------------------
class gpioWrapper

{
  public: 
  
         gpioWrapper(CALLBACKPIN p);

// --- public methods

CALLBACKPIN changePin=NULL;
CALLBACKENC changeEncoder=NULL;

     int start();
    void stop();
     int readpipe(char* buffer,int len);
     int setPin(int pin, int mode, int pullup,int longpush);
     int setEncoder(CALLBACKENC e);
    void writePin(int pin, int v);
     int openPipe();

// -- public attributes

      byte                TRACE=0x00;
      pid_t               pid = 0;
      int                 status;
      struct gpio_state   g[MAXGPIO];

      int                 indexGPIO=0;
      int                 inpipefd[2];
      int                 outpipefd[2];

      boolean             encoder=false;
      int                 clk;
      int                 dt;
      byte                MSW = 0;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="gpioWrapper";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

private:


};

#endif
//---------------------------------------------------------------------------------------------------
// gpio CLASS Implementation
//--------------------------------------------------------------------------------------------------
gpioWrapper::gpioWrapper(CALLBACKPIN p){


   if (p!=NULL) {changePin=p;}

// --- initial definitions

   indexGPIO=0;
   setWord(&MSW,RUN,false);
}
//---------------------------------------------------------------------------------------------------
// start operations (fork processes) Implementation
//--------------------------------------------------------------------------------------------------
int gpioWrapper::start() {

  setWord(&MSW,RUN,true);
  return 0;
}
//---------------------------------------------------------------------------------------------------
// writePin CLASS Implementation
//--------------------------------------------------------------------------------------------------
void gpioWrapper::writePin(int pin, int v) {

char buffer[256];

    if (pin <= 0 || pin >= MAXGPIO) {
       return;
    }

    if (v != 0 && v!= 1) {
       return;
    }

    if (getWord(MSW,RUN)==false) {
       return;
    }

    sprintf(buffer,"python /home/pi/PixiePi/bash/gpioset.py %d %d",pin,v);
    int rc=system(buffer);

    (TRACE>=0x00 ? fprintf(stderr,"%s::writePin write pin(%d) value(%d) command(%s) rc(%d)\n",PROGRAMID,pin,v,buffer,rc) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void gpioWrapper::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (getWord(MSW,RUN)==false) {
     return;
  }

  setWord(&MSW,RUN,false);
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
