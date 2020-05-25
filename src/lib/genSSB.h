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


#include <sys/shm.h>		//Used for shared memory

#include <sys/types.h>
#include <sys/stat.h>
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include "/home/pi/OrangeThunder/src/OT4D/transceiver.h"

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);

//----- SHARED MEMORY -----
struct shared_memory_struct {
	int  ptt_flag;
        bool ptt_signal;
        int  vox_flag;
        bool vox_signal;
	char common_data[1024];
};



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
//     int openPipe();

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


      void    *pshared = (void *)0;
//VARIABLES:

      struct shared_memory_struct *sharedmem;
      int    sharedmem_id;

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genSSB<Object>";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

const char   *mUSB="usb";
const char   *mAM="am";
const char   *mFM="fm";
const char   *mLSB="lsb";

      int    sharedmem_token=12345;

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
   sprintf(soundHW,"%s",(char*)"hw:Loopback_1,1,0");
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

//--------------------------------
//----- CREATE SHARED MEMORY -----
//--------------------------------
   (TRACE>=0x02 ? fprintf(stderr,"%s:genSSB() Creating shared memory...\n",PROGRAMID) : _NOP);
   sharedmem_id = shmget((key_t)sharedmem_token, sizeof(struct shared_memory_struct), 0666 | IPC_CREAT);		//<<<<< SET THE SHARED MEMORY KEY    (Shared memory key , Size in bytes, Permission flags)
//	Shared memory key
//		Unique non zero integer (usually 32 bit).  Needs to avoid clashing with another other processes shared memory (you just have to pick a random value and hope - ftok() can help with this but it still doesn't guarantee to avoid colision)
//	Permission flags
//		Operation permissions 	Octal value
//		Read by user 			00400
//		Write by user 			00200
//		Read by group 			00040
//		Write by group 			00020
//		Read by others 			00004
//		Write by others			00002
//		Examples:
//			0666 Everyone can read and write

   if (sharedmem_id == -1) {
      (TRACE>=0x00 ? fprintf(stderr, "%s:genSSB() Shared memory shmget() failed\n",PROGRAMID) : _NOP);
       exit(16);
   }

//Make the shared memory accessible to the program

   pshared = shmat(sharedmem_id, (void *)0, 0);
   if (pshared == (void *)-1) 	{
      (TRACE>=0x00 ? fprintf(stderr, "%s:genSSB() Shared memory shmat() failed\n",PROGRAMID) : _NOP);
      exit(16);
   }
   (TRACE>=0x00 ? fprintf(stderr,"%s:genSSB() Shared memory attached at %X\n",PROGRAMID,(int)pshared) : _NOP);

//Assign the shared_memory segment

   sharedmem = (struct shared_memory_struct *)pshared;


   sharedmem->ptt_flag=0x00;
   sharedmem->ptt_signal=false;
   sharedmem->vox_flag=0x00;
   sharedmem->vox_signal=false;

   (TRACE>=0x00 ? fprintf(stderr,"%s:genSSB() Finalized successfully\n",PROGRAMID) : _NOP);



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

//----- READ FROM SHARED MEMORY -----

  (TRACE>=0x00 ? fprintf(stderr,"%s::start() shared memory initialized ptt(%d/%s) vox(%d/%s)\n",PROGRAMID,sharedmem->ptt_flag,BOOL2CHAR(sharedmem->ptt_signal),sharedmem->vox_flag,BOOL2CHAR(sharedmem->vox_signal)) : _NOP);

// --- launch pipe

  pid = fork();
  (TRACE>=0x01 ? fprintf(stderr,"%s::start() starting pid(%d)\n",PROGRAMID,pid) : _NOP);


  if (pid == 0)
  {

// --- This is executed by the child only, output is being redirected
    (TRACE>=0x02 ? fprintf(stderr,"%s::start() <CHILD> thread pid(%d)\n",PROGRAMID,pid) : _NOP);

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
   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start()<Child> mode set to[%s]\n",PROGRAMID,MODE) : _NOP);

   char cmd_SHARE[16];
   char cmd_DEBUG[16];
   char cmd_stdERR[16];

   if (vox!=true) {
      sprintf(cmd_SHARE,"-m %d",sharedmem_token);
   } else {
      sprintf(cmd_SHARE," ");
   }

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

   sprintf(command,"arecord -c%d -r%d -D %s -fS16_LE - %s | sudo genSSB %s %s %s %s | sudo sendRF -i /dev/stdin %s -s %d -f %d  ",this->soundChannel,this->soundSR,this->soundHW,cmd_stdERR,strDDS,strVOX,cmd_SHARE,cmd_DEBUG,strDDS,this->sr,(int)f);
   (this->TRACE >= 0x01 ? fprintf(stderr,"%s::start()<Child> cmd[%s]\n",PROGRAMID,command) : _NOP);

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

}
//---------------------------------------------------------------------------------------------------
// openPipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
//int  genSSB::openPipe() {
//
//     return -1;
//}
//---------------------------------------------------------------------------------------------------
// openPipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::setPTT(bool v) {

  (this->TRACE>=0x01 ? fprintf(stderr,"%s::setPTT() setPTT(%s) signaling(%d)\n",PROGRAMID,BOOL2CHAR(v),v) : _NOP);
  setWord(&MSW,PTT,v);

  sharedmem->ptt_signal=v;
  sharedmem->ptt_flag=1;

  this->statePTT=v;
}

//---------------------------------------------------------------------------------------------------
// readpipe CLASS Implementation
//--------------------------------------------------------------------------------------------------
int genSSB::readpipe(char* buffer,int len) {

   if (sharedmem->vox_flag != 0) {
      sharedmem->vox_flag=0;
      this->stateVOX=sharedmem->vox_signal;
     (TRACE>=0x03 ? fprintf(stderr,"%s::readpipe() VOX signal received VOX(%s)\n",PROGRAMID,BOOL2CHAR(this->stateVOX)) : _NOP);

      if (vox==true) {if (changeVOX!=NULL) {changeVOX();}}
   }

   return 0;

}
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genSSB::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (getWord(MSW,RUN)==false) {
     return;
  }


//--------------------------------
//----- DETACH SHARED MEMORY -----
//--------------------------------
//Detach and delete

  if (shmdt(pshared) == -1) {
      (TRACE>=0x00 ? fprintf(stderr, "%s::stop() shmdt failed\n",PROGRAMID) : _NOP);
  } else {
      if (shmctl(sharedmem_id, IPC_RMID, 0) == -1) {
   	 (TRACE>=0x00 ? fprintf(stderr, "%s::stop() shmctl(IPC_RMID) failed\n",PROGRAMID) : _NOP);
      }
  }


  kill(pid, SIGKILL); //send SIGKILL signal to the child process

  waitpid(pid, &status, 0);
  setWord(&MSW,RUN,false);
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
