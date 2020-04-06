//--------------------------------------------------------------------------------------------------
// rtlfm receiver   (HEADER CLASS)
// wrapper to call rtl_fm from an object managing different extended functionalities
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef rtlfm_h
#define rtlfm_h

#define _NOP        (byte)0

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

#define MLSB   0x00
#define MUSB   0x01
#define MCW    0x02
#define MCWR   0x03
#define MAM    0x04
#define MWFM   0x06
#define MFM    0x08
#define MDIG   0x0A
#define MPKT   0x0C



typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();


//---------------------------------------------------------------------------------------------------
// SSB CLASS
//---------------------------------------------------------------------------------------------------
class rtlfm
{
  public: 
  
      rtlfm();
 void start();
 void stop();
  int readpipe(char* buffer,int len);
 void setFrequency(float f);
 void setMode(byte m);
     
      byte                TRACE=0x00;
      boolean             running;
      pid_t               pid = 0;
      int                 status;
      int                 inpipefd[2];
      int                 outpipefd[2];
      float               f;
      int                 sr;
      int                 so;
      int                 vol;
      int		  mode;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="rtlfm";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

const char   *mUSB="usb";
const char   *mAM="am";
const char   *mFM="fm";
const char   *mLSB="lsb";
private:

     char MODE[128];
     char FREQ[16];
   

};

#endif
//---------------------------------------------------------------------------------------------------
// rtlfm CLASS Implementation
//--------------------------------------------------------------------------------------------------
rtlfm::rtlfm(){

   pid=0;
   setMode(MUSB);
   setFrequency(14074000);
   sr=4000;
   so=4000;
   vol=0;
   running=false;
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::rtlfm() Initialization completed\n",PROGRAMID) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setFrequency CLASS Implementation
//--------------------------------------------------------------------------------------------------
void rtlfm::setFrequency(float f) {

   if (f==this->f) {
      return;
   }
   this->f=f;
   if (this->f >= 1000000) {
      sprintf(FREQ,"%gM",this->f/1000000);
   } else {
      sprintf(FREQ,"%gK",this->f/1000);
   }

  
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::setFrequency <FREQ=%s> len(%d)\n",PROGRAMID,FREQ,(unsigned)strlen(FREQ)) : _NOP);
   write(outpipefd[1], FREQ,(unsigned)strlen(FREQ)+1);

}
//---------------------------------------------------------------------------------------------------
// setMode CLASS Implementation
//--------------------------------------------------------------------------------------------------
void rtlfm::setMode(byte m) {

if ( m == this->mode) {
   return;
}

switch(m) {
           case MCW:
           case MCWR: 
           case MUSB:
                   {
  	             sprintf(MODE,"%s",mUSB);
		     sr=4000;
		     so=4000;
                     break;
                   }
           case MLSB:
                   {
		     sr=4000;
		     so=4000;
  	             sprintf(MODE,"%s",mLSB);
                     break;
                   }
	   case MAM:
                   { 
		     sr=4000;
		     so=4000;

  	             sprintf(MODE,"%s",mAM);
     		     break;
		   }
 	   case MWFM:
		   {
  	             sprintf(MODE,"%s","fm -o 4 -A fast -l 0 -E deemp ");
		     sr=170000;
	  	     so=32000;
	  	     break;
		   }
	   case MFM:
		   {
  	             sprintf(MODE,"%s",mFM);
		     sr=4000;
		     so=4000;
	  	     break;
		   }
	   case MDIG:
		   {
  	             sprintf(MODE,"%s",mUSB);
		     sr=4000;
		     so=4000;
	  	     break;
		   }
	   case MPKT:
		   {

  	             sprintf(MODE,"%s",mUSB);
		     sr=4000;
		     so=4000;
	  	     break;
		   }

       }

   if ( running == false) {
      this->mode=m;
      return;
   }

   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::setMode(%s)\n",PROGRAMID,MODE) : _NOP);
   this->stop();
   this->start();
   return;
}
//---------------------------------------------------------------------------------------------------
// start operations (fork processes) Implementation
//--------------------------------------------------------------------------------------------------
void rtlfm::start() {

char   command[256];
// --- create pipes

  pipe(inpipefd);
  fcntl(inpipefd[1],F_SETFL,O_NONBLOCK);

  pipe(outpipefd);
  fcntl(outpipefd[0],F_SETFL,O_NONBLOCK);

// --- launch pipe

  pid = fork();

  if (pid == 0)
  {

// --- This is executed by the child only, output is being redirected

    dup2(outpipefd[0], STDIN_FILENO);
    dup2(inpipefd[1], STDOUT_FILENO);
    dup2(inpipefd[1], STDERR_FILENO);

// --- ask kernel to deliver SIGTERM in case the parent dies

    prctl(PR_SET_PDEATHSIG, SIGTERM);


// --- format command

   sprintf(command,"sudo /home/pi/OrangeThunder/bin/rtl_fm -M %s -f %s -s %d -r %d -E direct | mplayer -nocache -af volume=%d -rawaudio samplesize=2:channels=1:rate=%d -demuxer rawaudio - ",MODE,FREQ,sr,so,vol,so); 
   (this->TRACE >= 0x02 ? fprintf(stderr,"%s::start() command(%s)\n",PROGRAMID,command) : _NOP);

// --- process being launch, which is a test conduit of rtl_fm, final version should have some fancy parameterization

    execl(getenv("SHELL"),"sh","-c",command,NULL);

// --- Nothing below this line should be executed by child process. If so, 
// --- it means that the execl function wasn't successfull, so lets exit:
    exit(1);
  }

//**************************************************************************
// The code below will be executed only by parent. You can write and read
// from the child using pipefd descriptors, and you can send signals to 
// the process using its pid by kill() function. If the child process will
// exit unexpectedly, the parent process will obtain SIGCHLD signal that
// can be handled (e.g. you can respawn the child process).
//**************************************************************************
  running=true;
  (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start() receiver process started\n",PROGRAMID) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// readpipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int rtlfm::readpipe(char* buffer,int len) {

 if (running == true) {
    return read(inpipefd[0],buffer,len);
 } else {
    return 0;
 }

}
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void rtlfm::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (running==false) {
     return;
  }

  kill(pid, SIGKILL); //send SIGKILL signal to the child process
  waitpid(pid, &status, 0);
  running=false;
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
