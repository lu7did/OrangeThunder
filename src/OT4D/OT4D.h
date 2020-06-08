/*
 * OrangeThunder(OT4D) / PixiePi(Pi4D)
 * Conditional construction of methods based on either OT4D or Pi4D directive 
 *-----------------------------------------------------------------------------
 * simple USB transceiver for the OrangeThunder/PixiePi project
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
 but with no intention whatsoever to be a replacement for it.
*/
// *************************************************************************************************
// *                           Language libraries                                                  *
// *************************************************************************************************
#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include<sys/wait.h>
#include<sys/prctl.h>
// *************************************************************************************************
// *                           Build time libraries                                                *
// *************************************************************************************************
#include "/home/pi/PixiePi/src/minIni/minIni.h"
#include "../OT/OT.h"
#include "../lib/genSSB.h"
#include "../lib/CAT817.h" 
#include "../lib/genVFO.h"

// *********************************************************************************************************
// * Personalization can be made either to the OrangeThunder project (OT4D) or PixiePi project (Pi4D)
// *********************************************************************************************************
// --- Program initialization
#ifdef OT4D

// --- OT4D specific includes
#include "../lib/rtlfm.h"

// --- Program initialization
const char   *PROGRAMID="OT4D";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

#endif

// *--- PixiePi's Pi4D specific includes

#ifdef Pi4D
#include "/home/pi/PixiePi/src/lib/LCDLib.h"

// --- Program initialization
const char   *PROGRAMID="Pi4D";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

LCDLib *lcd;
char*  LCD_Buffer;
int    lcd_light;

#endif
// *************************************************************************************************
// *                           Common Memory definitions                                           *
// *************************************************************************************************
char       timestr[16];
char       inChar[32];
boolean    fthread=false;
pthread_t  t=(pthread_t)-1;
pthread_t  pift8=(pthread_t)-1;
char       pcmd[1024];


// --- Signal handling and other common definitions
struct sigaction sigact;
char   HW[32];
bool   vox=true;

#ifdef OT4D
float  f=14074000;
int    vol=10;
#endif

#ifdef Pi4D
float  f=7074000;
int    vol=0;
#endif

int    anyargs=1;

// --- Define INI related variables
char inifile[80];
char iniStr[100];
long nIni;
int  sIni,kIni;
char iniSection[50];

// --- System control and IPC variables

byte   TRACE=0x02;    //should be set to 0x00 upon debugging and system test finalization
byte   MSW=0x00;
int    iqsend_token=0;
// *----------------------------------------------------------------*
// *                  ssb  processing                               *
// *----------------------------------------------------------------*
// -- - genSSB object
genSSB* usb=nullptr;
char   *usb_buffer;

// *----------------------------------------------------------------*
// *                  rtl-sdr processing                            *
// *----------------------------------------------------------------*
#ifdef OT4D

// --- rtlfm object (PixiePi doesn't use a rtl-sdr dongle but it's own direct conversion receiver)

rtlfm*  rtl=nullptr;
char    *rtl_buffer;
#endif


// *----------------------------------------------------------------*
// *             FT817 CAT Simulation support                       *
// *----------------------------------------------------------------*
// --- CAT object
void    CATchangeMode();      // Callback when CAT receives a mode change
void    CATchangeFreq();      // Callback when CAT receives a freq change
void    CATchangeStatus();    // Callback when CAT receives a status change
CAT817* cat=nullptr;
char    port[80];
long    catbaud=4800;
int     SNR;

// *--- vfo callback and object generation

void    SSBchangeVOX();
void    changeDDS(float f);
genVFO* vfo=nullptr;

//-**************************************************************************************************
//-                                        Program methods
//-**************************************************************************************************

//==================================================================================================
//                                  Infrastructure methods
//
//==================================================================================================

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
// manage system signals (usually to terminate, but do it gracefully)
// ======================================================================================================================
static void sighandler(int signum)
{

   (TRACE >= 0x00 ? fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum) : _NOP);
   setWord(&MSW,RUN,false);
   if (getWord(MSW,RETRY)==true) {
      (TRACE >= 0x00 ? fprintf(stderr, "\n%s:sighandler() Re-entering SIG(%d), force!\n",PROGRAMID,signum) : _NOP);
      exit(16);
   }
   setWord(&MSW,RETRY,true);

}
//--------------------------------------------------------------------------------------------------
// read_stdin without waiting for an enter to be pressed
//--------------------------------------------------------------------------------------------------
void* read_stdin(void * null) {
char c;
char buf[4];

       (TRACE>=0x03 ? fprintf(stderr,"%s:read_stdin() Keyboard thread started\n",PROGRAMID) : _NOP);

       /*
        * ioctl() would be better here; only lazy
        * programmers do it this way:
        */
        system("/bin/stty cbreak");        /* reacts to Ctl-C */
        system("/bin/stty -echo");         /* no echo */
        while (getWord(MSW,RUN)==true) {
          c = toupper(getchar());
          sprintf(inChar,"%c",c);
          setWord(&MSW,GUI,true);
        }
        return NULL;
}

//---------------------------------------------------------------------------------------------------
// writePin CLASS Implementation
//--------------------------------------------------------------------------------------------------
/*
void writePin(int pin, int v) {

    if (pin <= 0 || pin >= MAXGPIO) {
       return;
    }

    if (v != 0 && v!= 1) {
       return;
    }

    if (getWord(MSW,RUN)==false) {
       return;
    }

    (v==1 ? gpioWrite(GPIO_PTT,1) : gpioWrite(GPIO_PTT,0));
    (TRACE>=0x02 ? fprintf(stderr,"%s:writePin write pin(%d) value(%d)\n",PROGRAMID,pin,v) : _NOP);

}
*/
// *---------------------------------------------------------------------------------------------------------------------
// setPTT(boolean)
// handle PTT variations
// *---------------------------------------------------------------------------------------------------------------------
void setPTT(bool ptt) {

    (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT(%s)\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);
    if (ptt==true) {  //currently receiving now transmitting

    // *---------------------------------------------*
    // * Set transceiver into TX mode                *
    // *---------------------------------------------*
        usb->setPTT(ptt);
        usleep(10000);
        return;
    }

// *---------------------------------------------*
// * Establish PTT in receive mode               *
// *---------------------------------------------*
    usb->setPTT(ptt);
    usleep(10000);
    setWord(&MSW,PTT,ptt);
    (TRACE>=0x02 ? fprintf(stderr,"%s:setPTT() set PTT as(%s)\n",PROGRAMID,(getWord(MSW,PTT)==true ? "True" : "False")) : _NOP);
    return;
}
//---------------------------------------------------------------------------
// CATchangeFreq()
// CAT Callback when frequency changes
//---------------------------------------------------------------------------
void CATchangeFreq() {

// *--- Frequency can be changed only when receiving

  if (vfo->getPTT() == true) {
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) request while transmitting, ignored!\n",PROGRAMID,(int)cat->f) : _NOP);
     cat->f=vfo->get();
     return;
  }

// *--- Prevent some nasty early calls when the vfo hasn't been fully initialized yet
  
  if (vfo!=nullptr) {
     if (vfo->getmin() <= cat->f && vfo->getmax() >= cat->f) {
        (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeFreq() Frequency change to f(%d)\n",PROGRAMID,(int)cat->f) : _NOP);
        vfo->set(cat->f);
        return;
     }
  }

// *-- get change and process

  cat->f=vfo->get();
  (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeFreq() Frequency change is not allowed(%d)\n",PROGRAMID,(int)vfo->get()) : _NOP);

}
//-----------------------------------------------------------------------------------------------------------
// CATchangeMode
// Validate the new mode is a supported one
// At this point only CW,CWR,USB and LSB are supported
//-----------------------------------------------------------------------------------------------------------
void CATchangeMode() {

// *--- this is an USB transceiver, therefore an USB transceiver it is

  (TRACE>=0x02 ? fprintf(stderr,"%s:CATchangeMode() requested MODE(%d) not supported\n",PROGRAMID,cat->MODE) : _NOP);
  cat->MODE=vfo->MODE;
  return;

}

//------------------------------------------------------------------------------------------------------------
// CATchangeStatus
// Detect which change has been produced and operate accordingly
//------------------------------------------------------------------------------------------------------------
void CATchangeStatus() {

  (TRACE >= 0x03 ? fprintf(stderr,"%s:CATchangeStatus() FT817(%d) cat.FT817(%d)\n",PROGRAMID,vfo->FT817,cat->FT817) : _NOP);

// *-- Change PTT from the CAT interface

  if (getWord(cat->FT817,PTT) != vfo->getPTT()) {
     (TRACE>=0x02 ? fprintf(stderr,"%s:CATchangeStatus() PTT change request cat.FT817(%d) now is PTT(%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,PTT) ? "true" : "false") : _NOP);
     vfo->setPTT(getWord(cat->FT817,PTT));
     setWord(&MSW,PTT,getWord(cat->FT817,PTT));
  }

// *--- Changes in RIT,LOCK and SPLIT mostly for compatibility but not really supported

  if (getWord(cat->FT817,RITX) != getWord(vfo->FT817,RITX)) {        // RIT Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() RIT change request cat.FT817(%d) RIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,RITX) ? "true" : "false") : _NOP);
  }

  if (getWord(cat->FT817,LOCK) != getWord(vfo->FT817,LOCK)) {      // LOCK Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() LOCK change request cat.FT817(%d) LOCK changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,LOCK) ? "true" : "false") : _NOP);
  }

  if (getWord(cat->FT817,SPLIT) != getWord(vfo->FT817,SPLIT)) {    // SPLIT mode Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() SPLIT change request cat.FT817(%d) SPLIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,SPLIT) ? "true" : "false") : _NOP);
  }

// *--- WSJTX insist on changing the VFO so be it

  if (getWord(cat->FT817,VFO) != vfo->vfo) {        // VFO Changed
     vfo->setVFO(getWord(cat->FT817,VFO));
     (TRACE >=0x01 ? fprintf(stderr,"%s:CATchangeStatus() VFO changed to VFO(%d)\n",PROGRAMID,getWord(cat->FT817,VFO)) : _NOP);
  }
  return;

}


//--------------------------------------------------------------------------------------------------
// Callback to process SNR signal
//--------------------------------------------------------------------------------------------------
void CATgetRX() {

// *--- The SNR signal is available on the RTL-SDR therefore when provided by the dongle (thru rtl_fm) it's updated on the CAT interface 
#ifdef OT4D

    cat->RX=cat->snr2code(SNR);

#endif 

}
//--------------------------------------------------------------------------------------------------
// Callback to process TX signal
// (dummy implementation) no power is sensed either on PixiePi nor OrangeThunder
//--------------------------------------------------------------------------------------------------
void CATgetTX() {

}
// ======================================================================================================================
// SNR upcall change
// Callback from rtl_fm to get the signal to noise ratio level
// ======================================================================================================================
void changeSNR() {

#ifdef OT4D
     SNR=rtl->SNR;
#endif

}
//--------------------------------------------------------------------------------------------------
// callback for vfo (change Frequency)
// Called from the VFO object when the frequency has been changed
//--------------------------------------------------------------------------------------------------
void vfoChangeFreq(float f) {

char* b;
   b=(char*)malloc(128);
   vfo->vfo2str(vfo->vfo,b);
   if (cat!=nullptr) {
      cat->f=vfo->get(vfo->vfo);
   }
   (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeFreq() VFO(%s) f(%5.0f) fA(%5.0f) fB(%5.0f) PTT(%s)\n",PROGRAMID,b,vfo->get(),vfo->get(VFOA),vfo->get(VFOB),BOOL2CHAR(vfo->getPTT())) : _NOP);
   return;
}
//--------------------------------------------------------------------------------------------------
// callback for vfo (change mode)
// Called from the VFO object when the mode has been changed
//-------------------------------------------------------------------------------------------------
void vfoChangeMode(byte m) {

char* b;
   b=(char*)malloc(128);
   vfo->code2mode(m,b);

   if (cat!=nullptr) {
      cat->MODE=vfo->MODE;
   }

   (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeMode() mode(%s)\n",PROGRAMID,b) : _NOP);

}
//--------------------------------------------------------------------------------------------------
// callback for vfo (change status)
// called from the VFO object when the SPLIT,RIT or other variables has been changed (specially PTT)
//--------------------------------------------------------------------------------------------------
void vfoChangeStatus(byte S) {

   if (getWord(S,SPLIT)==true) {
      if(vfo==nullptr) {return;}
      (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeStatus() change SPLIT S(%s)\n",PROGRAMID,BOOL2CHAR(getWord(vfo->FT817,SPLIT))) : _NOP);
      if (cat!=nullptr) {
         setWord(&cat->FT817,SPLIT,getWord(vfo->FT817,SPLIT));
      }
   }
   if (getWord(S,RITX)==true) {
      if(vfo==nullptr) {return;}
      (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeStatus() change RIT S(%s)\n",PROGRAMID,BOOL2CHAR(getWord(vfo->FT817,RITX))) : _NOP);
      if (cat!=nullptr) {
         setWord(&cat->FT817,RITX,getWord(vfo->FT817,RITX));
      }
   }

   if (getWord(S,PTT)==true) {
      if (vfo==nullptr) {return;}
      (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeStatus() change PTT S(%s)\n",PROGRAMID,BOOL2CHAR(getWord(vfo->FT817,PTT))) : _NOP);
      if (cat!=nullptr) {
         setWord(&cat->FT817,PTT,getWord(vfo->FT817,PTT));
         setWord(&MSW,PTT,getWord(cat->FT817,PTT));
      }
      if (usb!=nullptr) {usb->setPTT(vfo->getPTT());}
   }
   if (getWord(S,VFO)==true) {
      if (vfo==nullptr) {return;}
      (TRACE>=0x02 ? fprintf(stderr,"%s:vfoChangeStatus() change VFO S(%s)\n",PROGRAMID,BOOL2CHAR(getWord(vfo->FT817,VFO))) : _NOP);
      if (cat!=nullptr) {
         setWord(&cat->FT817,VFO,getWord(vfo->FT817,VFO));
      }
   }

   if (cat!=nullptr) {cat->FT817=vfo->FT817;}
}
//--------------------------------------------------------------------------------------------------
// returns the time in a string format
//--------------------------------------------------------------------------------------------------
char* getTime() {

       time_t theTime = time(NULL);
       struct tm *aTime = localtime(&theTime);
       int hour=aTime->tm_hour;
       int min=aTime->tm_min;
       int sec=aTime->tm_sec;
       sprintf(timestr,"%02d:%02d:%02d",hour,min,sec);
       return (char*) &timestr;

}
// ======================================================================================================================
// VOX upcall signal
// upcall when the VOX has been activated
// ======================================================================================================================
void SSBchangeVOX() {

  (TRACE>=0x02 ? fprintf(stderr,"%s:SSBchangeVOX() received upcall from genSSB object state(%s)\n",PROGRAMID,BOOL2CHAR(usb->stateVOX)) : _NOP);
  vfo->setPTT(usb->stateVOX);

}
//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\
Usage: [-f Frequency] [-v volume] [-p portname] [-t tracelevel] \n\
-p            portname (default /tmp/ttyv0)\n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-t            tracelevel (0 to 3)\n\
-x            enable VOX control\n\
-m            enable shared memory IPC\n\
-v            sound card volume (-10 to 30)\n\
-?            help (this help).\n\
\n",PROGRAMID,PROG_VERSION,PROG_BUILD);

} /* end function print_usage */
// ======================================================================================================================
// MAIN
// Create IPC pipes, launch child process and keep communicating with it
// ======================================================================================================================
int main(int argc, char** argv)
{

  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);

#ifdef Pi4D    // in PixiePi run it as root always
  if (getuid()) {
      fprintf(stderr,"%s:main() %s\n",PROGRAMID,"This program must be run using sudo to acquire root privileges, terminating!");
      exit(16);
  }
#endif

// *---- Define system handlers

  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;


  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  signal(SIGPIPE, SIG_IGN);

  sprintf(port,CAT_PORT);
  sprintf(inifile,CFGFILE);

//---------------------------------------------------------------------------------
// reading INI files
//---------------------------------------------------------------------------------

#ifdef OT4D

  f=ini_getl("OT4D","FREQ",14074000,inifile);
  TRACE=ini_getl("OT4D","TRACE",0,inifile);
  vol=ini_getl("OT4D","VOL",10,inifile);
  nIni=ini_gets("OT4D", "PORT", "/tmp/ttyv0", port, sizearray(port), inifile);

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() INI  FREQ=%g\n",PROGRAMID,f) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main() INI  TRACE=%d\n",PROGRAMID,TRACE) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main() INI  VOL=%d\n",PROGRAMID,vol) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main() INI  PORT=%s\n",PROGRAMID,port) : _NOP);

#endif

// --- acquire memory areas

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize memory areas\n",PROGRAMID) : _NOP);
  usb_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));


// *--- define hardware interfaces

#ifdef OT4D
strcpy(HW,"hw:Loopback_1,1,0");
rtl_buffer=(char*)malloc(RTLSIZE*sizeof(unsigned char));
#endif

#ifdef Pi4D
strcpy(HW,"hw:1");
#endif

//---------------------------------------------------------------------------------
// arg_parse (parameters override previous configuration)
//---------------------------------------------------------------------------------
   while(1)
	{
	int ax = getopt(argc, argv, "p:f:t:v:hxm");
	if(ax == -1) 
	{
	  if(anyargs) break;
	  else ax='h'; //print usage and exit
        }
	anyargs = 1;

	switch(ax)
	{
	case 'p': // File name
             sprintf(port,optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- port(%s)\n",PROGRAMID,port) : _NOP);
	     break;
	case 'f': // Frequency
	     f = atof(optarg);
  	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- frequency(%10f)\n",PROGRAMID,f) : _NOP);
	     break;
	case 'v': // Volume
	     vol = atoi(optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- volume(%d)\n",PROGRAMID,vol) : _NOP);
             break;
	case 'x': // voX
	     vox=true;
	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- vox(%s)\n",PROGRAMID,BOOL2CHAR(vox)) : _NOP);
             break;
	case 'm': // shared memory IPC
	     iqsend_token=31415;
	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- IPC(%d)\n",PROGRAMID,iqsend_token) : _NOP);
             break;
	case 't': // tracelevel
	     TRACE = atoi(optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:main() args--- tracelevel(%d)\n",PROGRAMID,TRACE) : _NOP);
             break;
	case -1:
             break;
	case '?':
	     if (isprint(optopt) )
 	     {
 	        (TRACE>=0x00 ? fprintf(stderr, "%s:main()  unknown option `-%c'.\n",PROGRAMID,optopt) : _NOP);
 	     } 	else {
		(TRACE>=0x00 ?fprintf(stderr, "%s:main()  unknown option character `\\x%x'.\n",PROGRAMID,optopt) : _NOP);
  	     }
	     print_usage();
	     exit(1);
	     break;
	default:
   	     print_usage();
	     exit(1);
	     break;
	}/* end switch a */
	}/* end while getopt() */


#ifdef Pi4D
// *-----------------------------------------------------------------------------------------
// * Setup LCD Display (only PixiePi has one, so OT4D skip this)
// *-----------------------------------------------------------------------------------------
    LCD_Buffer=(char*) malloc(32);
    lcd=new LCDLib(NULL);

    lcd->begin(16,2);
    lcd->clear();
    lcd->createChar(0,TX);
    lcd_light=LCD_ON;
    lcd->backlight(true);
    lcd->setCursor(0,0);
  
    (TRACE>=0x02 ? fprintf(stderr,"%s %s:main() LCD display turned on\n",getTime(),PROGRAMID) : _NOP);

    sprintf(LCD_Buffer,"%s %s(%s)",PROGRAMID,PROG_VERSION,PROG_BUILD);
    lcd->println(0,0,LCD_Buffer);

    sprintf(LCD_Buffer,"  %5.1f KHz",f/1000);
    lcd->println(0,1,LCD_Buffer);

#endif

// *--- Initialize VFO sub-system

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize VFO sub-system interface\n",PROGRAMID) : _NOP);

  vfo=new genVFO(vfoChangeFreq,NULL,vfoChangeMode,vfoChangeStatus);
  vfo->TRACE=TRACE;
  vfo->FT817=0x00;
  vfo->MODE=MUSB;
  vfo->POWER=DDS_MAXLEVEL;
  vfo->setBand(VFOA,vfo->getBand(f));
  vfo->setBand(VFOB,vfo->getBand(f));
  vfo->set(VFOA,f);
  vfo->set(VFOB,f);
  vfo->setSplit(false);
  vfo->setRIT(VFOA,false);
  vfo->setRIT(VFOB,false);
  vfo->setVFO(VFOA);
  vfo->setPTT(false);
  vfo->setLock(false);

// --- Initialize USB generator

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize SSB generator interface\n",PROGRAMID) : _NOP);
  usb=new genSSB(SSBchangeVOX);  
  usb->TRACE=vfo->TRACE;
  usb->setFrequency(vfo->get());
  usb->setSoundChannel(CHANNEL);
  usb->setSoundSR(AFRATE);
  usb->setSoundHW(HW);
  usb->iqsend_token=iqsend_token;

//*--- OT assumes not to require a DDS function while receiving and to manage the PTT by itself (either using the VOX or the CAT)
#ifdef OT4D
  usb->dds=false;
  usb->vox=false;
#endif

//*--- PixiePi in turn assumes to require a DDS function while receiving and to manage the PTT using the VOX only to flight light
#ifdef Pi4D
  usb->dds=true;
  usb->vox=true;
#endif

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() Starting SSB generator interface\n",PROGRAMID) : _NOP);
  usb->start();

#ifdef OT4D

// --- Initialize and define rtl-sdr handling object (PixiePi uses it's own DC receiver)

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize RTL-SDR controller interface\n",PROGRAMID) : _NOP);
  rtl=new rtlfm();  
  rtl->TRACE=TRACE;
  rtl->setMode(vfo->getMode());
  rtl->setVol(vol);  
  rtl->setFrequency(f);
  rtl->changeSNR=changeSNR;
  rtl->start();

// --- creation of CAT object

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize CAT controller interface\n",PROGRAMID) : _NOP);
  cat=new CAT817(CATchangeFreq,CATchangeStatus,CATchangeMode,CATgetRX,CATgetTX);
  cat->FT817=vfo->FT817;
  cat->POWER=vfo->POWER;
  cat->f=vfo->get();
  cat->MODE=vfo->getMode();
  cat->TRACE=TRACE;
  cat->open(port,catbaud);
  cat->getRX=CATgetRX;

  setWord(&cat->FT817,AGC,false);
  setWord(&cat->FT817,PTT,vfo->getPTT());

#endif


#ifdef OT4D
  setWord(&cat->FT817,VFO,vfo->vfo);
#endif  

// -- create a thread to read the keyboard and start operations.

  pthread_create(&t, NULL, &read_stdin, NULL);
  setWord(&MSW,RUN,true);
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() start operation\n",PROGRAMID) : _NOP);

#ifdef OT4D
// --- Cycle the PTT to check the interface  
  setPTT(true);
  sleep(ONESEC);
#endif

// -- Establish reception mode
  setPTT(false);
  

//*-------------------------------------------------------------------------------------------*
//*                                 MAIN PROGRAM LOOP                                         *
//*-------------------------------------------------------------------------------------------*
  while(getWord(MSW,RUN)==true)
 
  {
    //*---------------------------------------------*
    //*           Process CAT commands              *
    //*---------------------------------------------*
#ifdef OT4D  // PixiePi (Pi4D) has no CAT support
    if (cat!=nullptr) {cat->get();}
#endif

#ifdef OT4D // OT4D receives thru RTL-SDR, Pi4D with its own DC receiver
    if (getWord(rtl->MSW,RUN)==true) {

    int rtl_read=rtl->readpipe(rtl_buffer,BUFSIZE);
        if (rtl_read>0) {
           rtl_buffer[rtl_read]=0x00;
           (TRACE>=0x02 ? fprintf(stderr,"%s",(char*)rtl_buffer) : _NOP);
        }
    }


#endif

// *--- When hit X exit the program (way to allow exit without a Ctl-C brute force method)

    if (getWord(MSW,GUI)==true) {
        setWord(&MSW,GUI,false);
        if (strcmp(inChar,"X")==0) {
           setWord(&MSW,RUN,false);
        } 
    }

// *--- Process information from the USB interface

    if (getWord(usb->MSW,RUN)==true) {
    int nread=usb->readpipe(usb_buffer,BUFSIZE);
        usleep(1000);
    }

//* -------------------[end of main loop ]--------------------------------
  }

#ifdef OT4D

// --- Normal termination save configuration parameters (only OT4D)

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() saving parameters\n",PROGRAMID) : _NOP);
  sprintf(iniStr,"%f",f);
  nIni = ini_puts("OT4D","FREQ",iniStr,inifile);

  sprintf(iniStr,"%d",TRACE);
  nIni = ini_puts("OT4D","TRACE",iniStr,inifile);

  sprintf(iniStr,"%d",vol);
  nIni = ini_puts("OT4D","VOL",iniStr,inifile);

  sprintf(iniStr,"%s",port);
  nIni = ini_puts("OT4D","PORT",iniStr,inifile);
#endif

// --- Normal termination kills the child first and wait for its termination

#ifdef Pi4D

//*--- Turn off LCD

  lcd->backlight(false);
  lcd->setCursor(0,0);
  lcd->clear();
  delete(lcd);

#endif

// --- Stop all threads and child processes 

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() stopping operations\n",PROGRAMID) : _NOP);
  usb->stop();
  delete(usb);

#ifdef OT4D

  if (cat!=nullptr) {cat->close();}
  if (rtl!=nullptr) {rtl->stop();}

  delete(cat);
  delete(rtl);
  delete(vfo);
#endif

}
