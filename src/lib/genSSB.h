//--------------------------------------------------------------------------------------------------
// genSSB transmitter   (HEADER CLASS)
// wrapper to call genSSB from an object managing different extended functionalities
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef genSSB_h
#define genSSB_h

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

#include <sys/types.h>
#include <sys/stat.h>


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


#define PTT_FIFO "/tmp/ptt_fifo"





//---------------------------------------------------------------------------------------------------
// SSB CLASS
//---------------------------------------------------------------------------------------------------
class genSSB
{
  public: 
  
         genSSB(CALLBACK vox);

CALLBACK changeVOX=NULL;
    void start();
    void stop();
     int readpipe(char* buffer,int len);
    void setFrequency(float f);
    void setSoundChannel(int c);
    void setSoundSR(int sr);
    void setSoundHW(char* hw);
    void setPTT(bool v);

    void setMode(byte m);
     
      byte                TRACE=0x00;
      boolean             running;
      pid_t               pid = 0;
      int                 status;
      int                 inpipefd[2];
      int                 outpipefd[2];
      float               f;
      int                 sr;
      int                 vol;
      int		  mode;
      int                 soundChannel;
      int                 soundSR;
      char*               soundHW;
      int                 ptt_fifo = -1;
      int		  result;
      bool                stateVOX;
      bool                statePTT;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genSSB";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
      char   PTTON[16];
      char   PTTOFF[16];

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
// genSSB CLASS Implementation
//--------------------------------------------------------------------------------------------------
genSSB::genSSB(CALLBACK vox){

   if (vox!=NULL) {changeVOX=vox;}

   stateVOX=false;
   statePTT=false;
   pid=0;
   soundSR=48000;
   soundHW=(char*)malloc(16*sizeof(int));
   setMode(MUSB);
   setFrequency(14074000);
   setSoundChannel(1);
   setSoundSR(48000);
   strcpy(soundHW,"Loopback");
   sr=6000;
   vol=0;
   running=false;

   sprintf(PTTON,"PTT=1\n");
   sprintf(PTTOFF,"PTT=0\n");

   fprintf(stderr,"Making FIFO...\n");
   result = mkfifo(PTT_FIFO, 0777);		//(This will fail if the fifo already exists in the system from the app previously running, this is fine)
   if (result == 0) {		//FIFO CREATED
      fprintf(stderr,"New FIFO created: %s\n", PTT_FIFO);
      (this->TRACE>=0x01 ? fprintf(stderr,"%s::genSSB() Initialization completed\n",PROGRAMID) : _NOP);
   } else {
      fprintf(stderr,"Failed to create FIFO %s\n",PTT_FIFO);
   }


}

//---------------------------------------------------------------------------------------------------
// setSoundChannel CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundChannel(int c) {
   this->soundChannel=c;
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::setSoundChannel() Soundchannel defined (%d)\n",PROGRAMID,this->soundChannel) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setSoundSR CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundSR(int sr) {
   this->soundSR=sr;
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::setSoundSR() Sound card sample rate(%d)\n",PROGRAMID,this->soundSR) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setSoundHW CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundHW(char* hw) {
   strcpy(this->soundHW,hw);
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::setSoundHW() Sound card Hardware(%s)\n",PROGRAMID,this->soundHW) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setFrequency CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setFrequency(float f) {

   if (f==this->f) {
      return;
   }
   this->f=f; 
   (this->TRACE>=0x01 ? fprintf(stderr,"%s::setFrequency <FREQ=%s> len(%d)\n",PROGRAMID,FREQ,(int)this->f) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setMode CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setMode(byte m) {

if ( m == this->mode) {
   return;
}

switch(m) {
           case MCW:
           case MCWR: 
	   case MAM:
	   case MDIG:
	   case MPKT:
           case MUSB:
                   {
  	             sprintf(MODE,"%s",mUSB);
		     sr=6000;
                     break;
                   }
           case MLSB:
                   {
		     sr=6000;
  	             sprintf(MODE,"%s",mLSB);
                     break;
                   }
 	   case MWFM:
	   case MFM:
                   {
	  	     break;
		   }

   }

   if (running == false) {
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
void genSSB::start() {

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
   //sprintf(command,"sudo /home/pi/OrangeThunder/bin/rtl_fm -M %s -f %s -s %d -r %d -E direct 2>/dev/null | mplayer -nocache -af volume=%d -rawaudio samplesize=2:channels=1:rate=%d -demuxer rawaudio - 2>/dev/null",MODE,FREQ,sr,so,vol,so); 

   sprintf(command,"arecord -c%d -r%d -D hw:%s,1 -fS16_LE - 2>/dev/null | /home/pi/OrangeThunder/bin/genSSB -f %d -s %d | sudo /home/pi/rpitx/sendiq -i /dev/stdin -s %d -f %d -t float 2>/dev/null",this->soundChannel,this->soundSR,this->soundHW,(int)f,this->sr,this->sr,(int)f);
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

  ptt_fifo = open(PTT_FIFO, (O_WRONLY));
  if (ptt_fifo != -1) {
     fprintf(stderr,"%s::start() opened ptt fifo\n",PROGRAMID);
     running=true;
     (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start() transmitter process started\n",PROGRAMID) : _NOP);

  } else {
     fprintf(stderr,"%s::start() error while opening ptt fifo\n",PROGRAMID);
  }


}
//---------------------------------------------------------------------------------------------------
// readpipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int genSSB::readpipe(char* buffer,int len) {

 if (running == true) {
    int rc=read(inpipefd[0],buffer,len);

    if (strcmp(buffer,"<<<VOX=1\n")==0) {
       stateVOX=true;
       if(changeVOX!=NULL) {changeVOX();}
       fprintf(stderr,"***PTT=1\n");
    }

    if (strcmp(buffer,"<<<VOX=0\n")==0) {
       stateVOX=false;
       if(changeVOX!=NULL) {changeVOX();}
       fprintf(stderr,"***PTT=0\n");
    }

    return rc;
 } else {
    return 0;
 }

}
//---------------------------------------------------------------------------------------------------
// setPTT CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setPTT(bool v) {

  if (v==true) {
     write(ptt_fifo,(void*)&PTTON,strlen(PTTON));
  } else {
     write(ptt_fifo,(void*)&PTTOFF,strlen(PTTOFF));
  }
  this->statePTT=v;
}
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (running==false) {
     return;
  }

  close(ptt_fifo);
  kill(pid, SIGKILL); //send SIGKILL signal to the child process
  waitpid(pid, &status, 0);
  running=false;
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
