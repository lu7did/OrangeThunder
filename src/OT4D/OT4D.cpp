/*
 * OT4D 
 *-----------------------------------------------------------------------------
 * simple USB transceiver for the OrangeThunder project
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
 This is an entry level USB transceiver to evaluate the best strategy
 to have rtl_fm and genSSB running at the same time.
 This program should be suitable to operate digital modes on a sigle
 frequency vaguely replicating the famous D4D transceiver by Adam Rong
*/

#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 
#include <pigpio.h>
#include "/home/pi/OrangeThunder/src/lib/genSSB.h"
#include "/home/pi/OrangeThunder/src/lib/rtlfm.h"
#include "/home/pi/PixiePi/src/lib/RPI.h" 
#include "/home/pi/PixiePi/src/lib/CAT817.h" 
#include "/home/pi/OrangeThunder/src/OT/OT.h"

// --- Program initialization

const char   *PROGRAMID="OT4D";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


// *************************************************************************************************
// *                           Common Memory definitions                                           *
// *************************************************************************************************

// --- IPC structures

struct sigaction sigact;
char   *HW;
float  f=14074000;

// --- System control objects

byte   TRACE=0x00;
byte   MSW=0x00;

// -- - genSSB object
genSSB* usb=nullptr;
char   *usb_buffer;

// --- rtlfm object

rtlfm*  rtl=nullptr;
char    *rtl_buffer;
// --- CAT object

void CATchangeMode();      // Callback when CAT receives a mode change
void CATchangeFreq();      // Callback when CAT receives a freq change
void CATchangeStatus();    // Callback when CAT receives a freq change

CAT817* cat;
byte FT817;
char  port[80];
long  catbaud=4800;

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
}
// ======================================================================================================================
// handle PTT variations
// ======================================================================================================================
void setPTT(bool ptt) {

    if (ptt==true) {  //currently receiving now transmitting
       fprintf(stderr,"%s:setPTT(%s) operating relay to TX position\n",PROGRAMID,BOOL2CHAR(ptt));
       int z = system("/usr/bin/python /home/pi/OrangeThunder/bash/turnon.py");
       usleep(10000);
       usb->setPTT(ptt);
    } else {          //currently transmitting, now receiving
       usb->setPTT(ptt);
       usleep(10000);
       int x = system("/usr/bin/python /home/pi/OrangeThunder/bash/turnoff.py");
    }
    setWord(&MSW,PTT,ptt);
    fprintf(stderr,"%s:setPTT() set PTT as(%s)\n",PROGRAMID,(getWord(MSW,PTT)==true ? "True" : "False"));
    return;
}
//---------------------------------------------------------------------------
// CATchangeFreq()
// CAT Callback when frequency changes
//---------------------------------------------------------------------------
void CATchangeFreq() {

  if (usb->statePTT == true) {
     fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) request while transmitting, ignored!\n",PROGRAMID,(int)cat->SetFrequency);
     cat->SetFrequency=f;
     return;
  }

  fprintf(stderr,"%s:CATchangeFreq() Frequency set(%d)\n",PROGRAMID,(int)f);

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

  if (getWord(cat->FT817,PTT) != usb->statePTT) {
     fprintf(stderr,"%s:CATchangeStatus() PTT change request cat.FT817(%d) now is PTT(%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,PTT) ? "true" : "false");
     setPTT(getWord(cat->FT817,PTT));
     setWord(&MSW,PTT,getWord(cat->FT817,PTT));
  }

  if (getWord(cat->FT817,RIT) != getWord(FT817,RIT)) {        // RIT Changed
     fprintf(stderr,"%s:CATchangeStatus() RIT change request cat.FT817(%d) RIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,RIT) ? "true" : "false");
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

  fprintf(stderr,"%s:SSBchangeVOX() received upcall from genSSB object state(%s)\n",PROGRAMID,BOOL2CHAR(usb->stateVOX));
  setPTT(usb->stateVOX);


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


  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
 
  signal(SIGPIPE, SIG_IGN);

// --- define control interface

  fprintf(stderr,"%s:main() initialize GPIO control interface\n",PROGRAMID);

  //if(gpioInitialise()<0) {
  //  fprintf(stderr,"%s:main Cannot initialize GPIO",PROGRAMID);
  //  exit(16);
  //}

  //gpioSetMode(GPIO_PTT, PI_OUTPUT);
  //usleep(100000);

  //gpioSetPullUpDown(GPIO_PTT,PI_PUD_UP);
  //usleep(100000);

  //gpioWrite(GPIO_PTT, 0);
  //usleep(100000);

// --- memory areas

  fprintf(stderr,"%s:main() initialize memory areas\n",PROGRAMID);
  usb_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));
  rtl_buffer=(char*)malloc(RTLSIZE*sizeof(unsigned char));

// --- ALSA loopback support 

  fprintf(stderr,"%s:main() initialize ALSA parameters\n",PROGRAMID);
  HW=(char*)malloc(16*sizeof(unsigned char));
  sprintf(HW,"Loopback");



// --- define rtl-sdr handling objects

  fprintf(stderr,"%s:main() initialize RTL-SDR controller interface\n",PROGRAMID);

  rtl=new rtlfm();  
  rtl->TRACE=TRACE;
  rtl->setMode(MUSB);
  rtl->setVol(10);  
  rtl->setFrequency(14074000);
  rtl->start();


// --- USB generator 

  fprintf(stderr,"%s:main() initialize SSB generator interface\n",PROGRAMID);

  usb=new genSSB(SSBchangeVOX);  
  usb->TRACE=TRACE;
  usb->setFrequency(f);
  usb->setSoundChannel(CHANNEL);
  usb->setSoundSR(AFRATE);
  usb->setSoundHW(HW);
  usb->start();

// --- creation of CAT object

  fprintf(stderr,"%s:main() initialize CAT controller interface\n",PROGRAMID);
  FT817=0x00;
  sprintf(port,"/tmp/ttyv0");

  cat=new CAT817(CATchangeFreq,CATchangeStatus,CATchangeMode,CATgetRX,CATgetTX);
  cat->FT817=FT817;
  cat->POWER=7;
  cat->SetFrequency=f;
  cat->MODE=MUSB;
  cat->TRACE=TRACE;
  cat->open(port,catbaud);

  setWord(&cat->FT817,AGC,false);
  setWord(&cat->FT817,PTT,getWord(MSW,PTT));

// -- establish loop condition
  
  
  setWord(&MSW,RUN,true);
  fprintf(stderr,"%s:main() start operation\n",PROGRAMID);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  

  setPTT(true);
  sleep(1);
  setPTT(false);
  //rtl->setFrequency(14074000);
  
  while(getWord(MSW,RUN)==true)
 
  {
    cat->get();

    if (getWord(rtl->MSW,RUN)==true) {
    int rtl_read=rtl->readpipe(rtl_buffer,1024);
        if (rtl_read>0) {
           rtl_buffer[rtl_read]=0x00;
           fprintf(stderr,"%s",(char*)rtl_buffer);
        }
    }
 
    if (getWord(usb->MSW,RUN)==true) {
    int usb_read=usb->readpipe(usb_buffer,1024);
        if (usb_read>0) {
           usb_buffer[usb_read]=0x00;
           fprintf(stderr,"%s",(char*)usb_buffer);
        }
    }

  }

// --- Normal termination kills the child first and wait for its termination
  fprintf(stderr,"%s:main() stopping operations\n",PROGRAMID);
  cat->close();
  usb->stop();
  rtl->stop();
}