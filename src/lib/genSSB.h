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
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include "/home/pi/OrangeThunder/src/OT4D/transceiver.h"

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);

//---------------------------------------------------------------------------------------------------
// SSB CLASS
//---------------------------------------------------------------------------------------------------
class genSSB
{
  public: 

         genSSB(CALLBACK vox);

// --- public methods

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
     int openPipe();

// -- public attributes

      byte    TRACE=0x00;
      pid_t   pid = 0;
      int     status;

      int     inpipefd[2];
      int     outpipefd[2];

      float   f;
      int     sr;
      int     vol;
      int     mode;
      int     soundChannel;
      int     soundSR;
      char    soundHW[64];
      int     ptt_fifo = -1;
      int     result;
      bool    stateVOX;
      bool    statePTT;
      bool    dds;
      bool    vox;
      byte    MSW = 0;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genSSB";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

const char   *mUSB="usb";
const char   *mAM="am";
const char   *mFM="fm";
const char   *mLSB="lsb";

private:

     char     MODE[128];
     char     FREQ[16];
     char     PTTON[16];
     char     PTTOFF[16];

};

#endif
//---------------------------------------------------------------------------------------------------
// genSSB CLASS Implementation
//--------------------------------------------------------------------------------------------------
genSSB::genSSB(CALLBACK v){

// -- VOX callback

   if (v!=NULL) {changeVOX=v;}

// --- initial definitions

   stateVOX=false;
   statePTT=false;
   dds=false;
   vox=false;
   pid=0;

   soundSR=48000;
   setSoundChannel(1);
   setSoundSR(48000);

   setMode(MUSB);
   setFrequency(FREQUENCY);
   sr=6000;
   vol=0;

   sprintf(PTTON,"PTT=1\n");
   sprintf(PTTOFF,"PTT=0\n");

#ifdef OT4D
   sprintf(soundHW,"%s",(char*)"hw:Loopback,1,0");
#endif

#ifdef Pi4D
   sprintf(soundHW,"%s",(char*)"hw:1");
#endif

   setWord(&MSW,RUN,false);

// --- Initialize command duct with genSSB

   (this->TRACE>=0x02 ? fprintf(stderr,"%s::genSSB() Making FIFO...\n",PROGRAMID) : _NOP);
   result = mkfifo("/tmp/ptt_fifo", 0666);		//(This will fail if the fifo already exists in the system from the app previously running, this is fine)
   if (result == 0) {	    	                        //FIFO CREATED
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::genSSB() Initialization completed new FIFO (%s) created\n",PROGRAMID,PTT_FIFO) : _NOP);
   } else {
      (this->TRACE>=0x00 ? fprintf(stderr,"%s::genSSB() Error during of command FIFO(%s), aborting\n",PROGRAMID,PTT_FIFO) : _NOP);
      exit(16);
   }

}

//---------------------------------------------------------------------------------------------------
// setSoundChannel CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundChannel(int c) {

   this->soundChannel=c;
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSoundChannel() Soundchannel defined (%d)\n",PROGRAMID,this->soundChannel) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setSoundSR CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundSR(int sr) {

   this->soundSR=sr;
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSoundSR() Sound card sample rate(%d)\n",PROGRAMID,this->soundSR) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setSoundHW CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setSoundHW(char* hw) {

   strcpy(this->soundHW,hw);
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSoundHW() Sound card Hardware(%s)\n",PROGRAMID,this->soundHW) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setFrequency CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setFrequency(float f) {

   if (f==this->f) {
      return;
   }
   this->f=f; 
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setFrequency <FREQ=%d>\n",PROGRAMID,(int)this->f) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setMode CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setMode(byte m) {

   if ( m == this->mode) {
      return;
   }

   this->mode=m;
   if (getWord(MSW,RUN) == false) {
      return;
   }

   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::setMode(%s), ignored actually\n",PROGRAMID,MODE) : _NOP);
   return;
}
//---------------------------------------------------------------------------------------------------
// start operations (fork processes) Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::start() {


char   command[256];
// --- create pipes
  (TRACE>=0x01 ? fprintf(stderr,"%s::start() starting tracelevel(%d) DDS(%s)\n",PROGRAMID,TRACE,BOOL2CHAR(dds)) : _NOP);

  pipe(inpipefd);
  fcntl(inpipefd[1],F_SETFL,O_NONBLOCK);
  fcntl(inpipefd[0],F_SETFL,O_NONBLOCK);

  pipe(outpipefd);
  fcntl(outpipefd[0],F_SETFL,O_NONBLOCK);

// --- launch pipe

  pid = fork();
  (TRACE>=0x01 ? fprintf(stderr,"%s::start() starting pid(%d)\n",PROGRAMID,pid) : _NOP);


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


   switch(this->mode) {
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
   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start() mode set to[%s]\n",PROGRAMID,MODE) : _NOP);

   char cmd_DEBUG[16];
   char cmd_stdERR[16];

   if (this->TRACE>=0x02) {
      sprintf(cmd_DEBUG," -t %d ",this->TRACE);
   } else {
     sprintf(cmd_DEBUG," ");
   }

   if (this->TRACE>=0x03) {
      sprintf(cmd_stdERR,"%s"," 2>/dev/null ");
   } else {
     sprintf(cmd_stdERR," ");
   }


   char strVOX[16];
   if (vox==true) {
      sprintf(strVOX,"%s",(char*)"-x -v 2");
   } else {
      sprintf(strVOX,"%s",(char*)" ");
   }

   char strDDS[16];
   if (dds==true) {
      sprintf(strDDS,"%s",(char*)"-d ");
   } else {
      sprintf(strDDS,"%s",(char*)" ");
   }


   sprintf(command,"arecord -c%d -r%d -D %s -fS16_LE - %s | sudo genSSB %s %s %s | sudo sendRF -i /dev/stdin %s -s %d -f %d  ",this->soundChannel,this->soundSR,this->soundHW,cmd_stdERR,strDDS,strVOX,cmd_DEBUG,strDDS,this->sr,(int)f);
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

 (TRACE>=0x02 ? fprintf(stderr,"%s::start() <PARENT> Opening FIFO pipe pid(%d)\n",PROGRAMID,pid) : _NOP);
// ptt_fifo = open("/tmp/ptt_fifo", (O_WRONLY));
// if (ptt_fifo != -1) {
//    (this->TRACE>=0x01 ? fprintf(stderr,"%s::start() opened ptt fifo(%s)\n",PROGRAMID,PTT_FIFO) : _NOP);
// } else {
//   (this->TRACE>=0x00 ? fprintf(stderr,"%s::start() error while opening ptt fifo error(%d), aborting\n",PROGRAMID,ptt_fifo) : _NOP);;
//    exit(16);
// }

  setWord(&MSW,RUN,true);

}
//---------------------------------------------------------------------------------------------------
// openPipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int  genSSB::openPipe() {

     return -1;
}
//---------------------------------------------------------------------------------------------------
// openPipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setPTT(bool v) {

  (this->TRACE>=0x00 ? fprintf(stderr,"%s::setPTT() setPTT(%s) sending PTT=%d\n",PROGRAMID,BOOL2CHAR(v),v) : _NOP);
  setWord(&MSW,PTT,v);

//  if (v==true) {
//     write(ptt_fifo,(void*)&PTTON,strlen(PTTON));
//  } else {
//     write(ptt_fifo,(void*)&PTTOFF,strlen(PTTOFF));
//  }

  this->statePTT=v;
}

//---------------------------------------------------------------------------------------------------
// readpipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int genSSB::readpipe(char* buffer,int len) {

   
    int rc=read(inpipefd[0],buffer,len);

    if (rc<=0) {
       return 0;
    }
     buffer[rc]=0x00;
    (TRACE>=0x03 ? fprintf(stderr,"%s::readpipe() Buffer(%s) len(%d)\n",PROGRAMID,buffer,rc) : _NOP);

char * token = strtok(buffer, "\n");

   // loop through the string to extract all other tokens
   while( token != NULL ) {

    if (strcmp(token,"VOX=1")==0) {
        if (vox==true) {
           this->stateVOX=true;
           if ( changeVOX!=NULL ) {changeVOX();}
              (TRACE>=0x02 ? fprintf(stderr,"genSSB::readpipe() received VOX=1 signal from child\n") : _NOP);
        } else {
          stateVOX=false;
        }
    }

    if (strcmp(token,"VOX=0")==0) {
       if (vox==true) {
          this->stateVOX=false;
          if(changeVOX!=NULL) {changeVOX();}
          (TRACE>=0x02 ? fprintf(stderr,"genSSB::readpipe() received VOX=0 signal from child\n") : _NOP);
       } else {
         this->stateVOX=false;
       }
    }
   (TRACE>=0x03 ? fprintf(stderr,"genSSB::readpipe() parsed token(%s)\n",token) : _NOP);
    token = strtok(NULL, "\n");
   }

   return rc;

}
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (getWord(MSW,RUN)==false) {
     return;
  }

  close(ptt_fifo);
  kill(pid, SIGKILL); //send SIGKILL signal to the child process
  waitpid(pid, &status, 0);
  setWord(&MSW,RUN,false);
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
