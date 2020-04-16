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
#include "/home/pi/OrangeThunder/src/OT/OT.h"

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACKPIN)(int pin,int state);

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
     int start();
    void stop();
     int readpipe(char* buffer,int len);
     int setPin(int pin, int mode, int pullup,int longpush);
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

      byte                MSW = 0;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="gpio";
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

   for (int i=0;i<MAXGPIO;i++) {
      g[i].active=false;
      g[i].mode  =false;
      g[i].pullup=false;
      g[i].longpush=false;
   }
   indexGPIO=0;
   setWord(&MSW,RUN,false);
}
//---------------------------------------------------------------------------------------------------
// start operations (fork processes) Implementation
//--------------------------------------------------------------------------------------------------
int gpioWrapper::start() {

  if (indexGPIO==0) {
    (TRACE>=0x02 ? fprintf(stderr,"%s::start() No GPIO pin defined\n",PROGRAMID) : _NOP);
     return -1;
  }

char   command[256];
char   ports[128];

// --- create pipes

  pipe(inpipefd);
  fcntl(inpipefd[1],F_SETFL,O_NONBLOCK);
  fcntl(inpipefd[0],F_SETFL,O_NONBLOCK);

  pipe(outpipefd);
  fcntl(outpipefd[0],F_SETFL,O_NONBLOCK);
  //fcntl(outpipefd[1],F_SETFL,O_NONBLOCK);

// --- launch pipe

  pid = fork();
  (TRACE>=0x02 ? fprintf(stderr,"%s::start() starting pid(%d)\n",PROGRAMID,pid) : _NOP);

  if (pid == 0)
  {

// --- This is executed by the child only, output is being redirected
    (TRACE>=0x02 ? fprintf(stderr,"%s::start() <CHILD> thread pid(%d)\n",PROGRAMID,pid) : _NOP);
    dup2(outpipefd[0], STDIN_FILENO);
    dup2(inpipefd[1], STDOUT_FILENO);
    dup2(inpipefd[1], STDERR_FILENO);

// --- ask kernel to deliver SIGTERM in case the parent dies

    prctl(PR_SET_PDEATHSIG, SIGTERM);


// --- format command

char cmd[16];

   ports[0]=0x00;
   for (int i=0; i<MAXGPIO; i++) {
       if (g[i].active==true) {
          sprintf(cmd,"-p %d:%d:%d:%d ",i,(g[i].mode==false ? 0 : 1),(g[i].pullup==false ? 0 : 1),(g[i].longpush==false ? 0 : 1));
          strcat(ports,cmd);
          (TRACE>=0x02 ? fprintf(stderr,"%s::start() added port(%d) to command\n",PROGRAMID,i) : _NOP);
       }
   }

   if (strlen(ports)==0) {
      (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start() cmd[%s] EMPTY, abandoning\n",PROGRAMID,command) : _NOP);
      return -1;
   }


   if (this->TRACE>0) {
      sprintf(cmd," -v %d",this->TRACE);
   } else {
      sprintf(cmd," ");
   }
      
   sprintf(command,"sudo /home/pi/OrangeThunder/bin/gpio %s %s",ports,cmd);
   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start() cmd[%s]\n",PROGRAMID,command) : _NOP);

// --- process being launch 

    execl(getenv("SHELL"),"sh","-c",command,NULL);

// --- Nothing below this line should be executed by child process. If so, 
// --- it means that the execl function wasn't successfull, so lets exit:

    exit(1);
  }

// ******************************************************************************************************
// The code below will be executed only by parent. You can write and read
// from the child using pipefd descriptors, and you can send signals to 
// the process using its pid by kill() function. If the child process will
// exit unexpectedly, the parent process will obtain SIGCHLD signal that
// can be handled (e.g. you can respawn the child process).
// *******************************************************************************************************

  setWord(&MSW,RUN,true);
  return 0;
}
//---------------------------------------------------------------------------------------------------
// openPipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int  gpioWrapper::openPipe() {

     return -1;
}
//---------------------------------------------------------------------------------------------------
// setPin CLASS Implementation
//--------------------------------------------------------------------------------------------------
int gpioWrapper::setPin(int pin, int mode, int pullup,int longpush) {

    if(pin <= 0 || pin >= MAXGPIO) {
      (TRACE>=0x00 ? fprintf(stderr,"%s::setPin invalid pin(%d)\n",PROGRAMID,pin) : _NOP);
      return -1;
    }

    g[pin].active=true;
    (mode == 1 ? g[pin].mode=true : g[pin].mode=false);
    (pullup == 1 ? g[pin].pullup=true : g[pin].pullup=false);
    (longpush == 1 ? g[pin].longpush=true : g[pin].longpush=false);

    indexGPIO++;
    (TRACE>=0x02 ? fprintf(stderr,"%s::setPin set pin(%d) mode(%s) pull(%s) long(%s)\n",PROGRAMID,pin,BOOL2CHAR(mode),BOOL2CHAR(pullup),BOOL2CHAR(longpush)) : _NOP);
    return 0;
}
//---------------------------------------------------------------------------------------------------
// readpipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int gpioWrapper::readpipe(char* buffer,int len) {

    if (getWord(MSW,RUN) != true) {
       (this->TRACE >= 0x01 ? fprintf(stderr,"%s::readpipe() function called without object running\n",PROGRAMID) : _NOP);
       return 0;
    }
   
    int rc=read(inpipefd[0],buffer,len);
    if (rc<=0) {
       return 0;
    }
    buffer[rc]=0x00;

    if (strcmp(buffer,"GPIO20=0\n")==0) {
       if (changePin != NULL) {
          changePin(20,0);
       }
    }
    if (strcmp(buffer,"GPIO20=1\n")==0) {
       if (changePin != NULL) {
          changePin(20,1);
       }
    }

    return rc;

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

    sprintf(buffer,"GPIO%d=%d\n",pin,v);
    write(outpipefd[1],buffer,strlen(buffer));
    (TRACE>=0x02 ? fprintf(stderr,"%s::writePin write pin(%d) value(%d) command(%s)\n",PROGRAMID,pin,v,buffer) : _NOP);

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
