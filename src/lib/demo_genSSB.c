/*
 * demo_genSSB 
 *-----------------------------------------------------------------------------
 * demo program for the genSSB program component
 * Copyright (C) 2020 by Pedro Colla <lu7did@gmail.com>
 * ----------------------------------------------------------------------------
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
 *
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

/*
 This is a proof of concept test platform to evaluate the best strategy
 to start and stop rtl_fm running in the background and to pass 
 parameters to it during run-time.
*/

#define Pi4D


#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 
//#include <pigpio.h>
#include "genSSB.h"
//#include "/home/pi/PixiePi/src/lib/RPI.h" 
#include "CAT817.h" 

#define BUFSIZE 1024

// --- IPC structures

genSSB* g=nullptr;
struct  sigaction sigact;
char    *buffer;
char    HW[32];

#ifdef OT4D
flost   f=14074000;
#endif

#ifdef Pi4D
float   f=7074000;
#endif

byte    TRACE=0x02;
byte    MSW=0x00;

// --- CAT object

void CATchangeMode();
void CATchangeFreq();
void CATchangeStatus();

CAT817* cat;
byte FT817;

char  port[80];
long  catbaud=4800;

const char   *PROGRAMID="demo_genSSB";
const char   *PROG_VERSION="1.0";

#ifdef OT4D
const char   *PROG_BUILD="00 {OT4D}";
#endif 

#ifdef Pi4D
const char   *PROG_BUILD="00 {Pi4D}";
#endif

const char   *COPYRIGHT="(c) LU7DID 2019,2020";
int    iqsend_token=31415;
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
    if (getWord(MSW,RETRY)==true) {
        fprintf(stderr, "\n%s:sighandler() Signal caught(%d), re-entrance, abort!!\n",PROGRAMID,signum);
        exit(16);
    }
    setWord(&MSW,RETRY,true);
   
}
// ======================================================================================================================
// handle PTT variations
// ======================================================================================================================
void setPTT(bool ptt) {

    if (g==nullptr) {return;}
    g->setPTT(ptt);
    setWord(&MSW,PTT,ptt);
    fprintf(stderr,"%s:setPTT() set PTT as(%s)\n",PROGRAMID,(getWord(MSW,PTT)==true ? "True" : "False"));
    usleep(10000);
    return;
}
//---------------------------------------------------------------------------
// CATchangeFreq()
// CAT Callback when frequency changes
//---------------------------------------------------------------------------
void CATchangeFreq() {

  if (g==nullptr) {return;}
  if (g->statePTT == true) {
     fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) request while transmitting, ignored!\n",PROGRAMID,(int)cat->f);
     cat->f=f;
     return;
  }

  fprintf(stderr,"%s:CATchangeFreq() Frequency set to SetFrequency(%d)\n",PROGRAMID,(int)f);

}
//-----------------------------------------------------------------------------------------------------------
// CATchangeMode
// Validate the new mode is a supported one
// At this point only CW,CWR,USB and LSB are supported
//-----------------------------------------------------------------------------------------------------------
void CATchangeMode() {

  fprintf(stderr,"%s:CATchangeMode() requested MODE(%d) not supported\n",PROGRAMID,cat->MODE);
  cat->MODE=MUSB;
  return;

}

//------------------------------------------------------------------------------------------------------------
// CATchangeStatus
// Detect which change has been produced and operate accordingly
//------------------------------------------------------------------------------------------------------------
void CATchangeStatus() {

  fprintf(stderr,"%s:CATchangeStatus() FT817(%d) cat.FT817(%d)\n",PROGRAMID,FT817,cat->FT817);

  if (getWord(cat->FT817,PTT) != g->statePTT) {
     fprintf(stderr,"%s:CATchangeStatus() PTT change request cat.FT817(%d) now is PTT(%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,PTT) ? "true" : "false");
     setPTT(getWord(cat->FT817,PTT));
     setWord(&MSW,PTT,getWord(cat->FT817,PTT));
  }

  if (getWord(cat->FT817,RITX) != getWord(FT817,RITX)) {        // RIT Changed
     fprintf(stderr,"%s:CATchangeStatus() RIT change request cat.FT817(%d) RIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,RITX) ? "true" : "false");
  }

  if (getWord(cat->FT817,LOCK) != getWord(FT817,LOCK)) {      // LOCK Changed
     fprintf(stderr,"%s:CATchangeStatus() LOCK change request cat.FT817(%d) LOCK changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,LOCK) ? "true" : "false");
  }

  if (getWord(cat->FT817,SPLIT) != getWord(FT817,SPLIT)) {    // SPLIT mode Changed
     fprintf(stderr,"%s:CATchangeStatus() SPLIT change request cat.FT817(%d) SPLIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,SPLIT) ? "true" : "false");
  }

  if (getWord(cat->FT817,VFO) != getWord(FT817,VFO)) {        // VFO Changed
     fprintf(stderr,"%s:CATchangeStatus() VFO change request not supported\n",PROGRAMID);
  }

  FT817=cat->FT817;
  return;

}
//--------------------------------------------------------------------------------------------------
// Stubs for callback not implemented yed
//--------------------------------------------------------------------------------------------------

void CATgetRX() {
}
void CATgetTX() {
}
// ======================================================================================================================
// VOX upcall signal
// ======================================================================================================================
void SSBchangeVOX() {

  fprintf(stderr,"%s:SSBchangeVOX() received upcall from genSSB object state(%s)\n",PROGRAMID,BOOL2CHAR(g->stateVOX));
  setPTT(g->stateVOX);

}
// ======================================================================================================================
// MAIN
// Create IPC pipes, launch child process and keep communicating with it
// ======================================================================================================================
int main(int argc, char** argv)
{

  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);

  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

//*--- Define the rest of the signal handlers, basically as termination exceptions

  fprintf(stderr,"%s:main() initialize interrupt handling system\n",PROGRAMID);
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

// --- memory areas

  fprintf(stderr,"%s:main() initialize memory areas\n",PROGRAMID);
  buffer=(char*)malloc(2048*sizeof(unsigned char));

#ifdef OT4D
  strcpy(HW,"hw:Loopback,1,0");
#endif

#ifdef Pi4D
  strcpy(HW,"hw:1");
#endif
// --- USB generator 

  fprintf(stderr,"%s:main() initialize SSB generator interface\n",PROGRAMID);

  g=new genSSB(SSBchangeVOX);  
  g->TRACE=TRACE;
  g->setFrequency(f);
  g->setSoundChannel(CHANNEL);
  g->setSoundSR(AFRATE);
  g->setSoundHW(HW);
  g->vox=true;
  g->dds=false;
  g->iqsend_token=iqsend_token;

  g->start();

// --- creation of CAT object

  fprintf(stderr,"%s:main() initialize CAT controller interface\n",PROGRAMID);

  FT817=0x00;
  sprintf(port,"/tmp/ttyv0");

  cat=new CAT817(CATchangeFreq,CATchangeStatus,CATchangeMode,CATgetRX,CATgetTX);

  cat->FT817=FT817;
  cat->POWER=7;
  cat->f=f;
  cat->MODE=MUSB;
  cat->TRACE=TRACE;

  cat->open(port,catbaud);
  setWord(&cat->FT817,AGC,false);
  setWord(&cat->FT817,PTT,false);

// -- establish loop condition
  
  setWord(&MSW,RUN,true);
  fprintf(stderr,"%s:main() start operation\n",PROGRAMID);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(getWord(MSW,RUN)==true)
  
  {
    cat->get(); 
int nread=g->readpipe(buffer,BUFSIZE);
    usleep(1000);    
  }

// --- Normal termination kills the child first and wait for its termination
  fprintf(stderr,"%s:main() stop operations and terminate\n",PROGRAMID);
  g->stop();
  cat->close();
  delete(g);
  delete(cat);
}
