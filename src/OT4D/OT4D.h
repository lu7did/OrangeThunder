
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
// *************************************************************************************************
// *                           Conditional Building for OT4                                        *
// * - usb only                                                                                    *
// * - reception thru rtl-sdr                                                                      *
// *************************************************************************************************


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

#include "/home/pi/PixiePi/src/minIni/minIni.h"


// --- Program initialization
#ifdef OT4D

// --- OT4D specific includes
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include "/home/pi/OrangeThunder/src/lib/gpioWrapper.h"
#include "/home/pi/OrangeThunder/src/lib/genSSB.h"
#include "/home/pi/PixiePi/src/lib/CAT817.h" 
#include "/home/pi/OrangeThunder/src/lib/rtlfm.h"
#include "/home/pi/OrangeThunder/src/lib/genVFO.h"


const char   *PROGRAMID="OT4D";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
#endif

#ifdef Pi4D
// --- OT4D specific includes
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#include "/home/pi/PixiePi/src/lib/DDS.h"

// --- Program initialization
const char   *PROGRAMID="Pi4D";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

#endif
// *************************************************************************************************
// *                           Common Memory definitions                                           *
// *************************************************************************************************

// --- IPC structures
struct sigaction sigact;
char   *HW;
float  f=14074000;
int    anyargs=1;

// --- Define INI related variables
char inifile[80];
char iniStr[100];
long nIni;
int  sIni,kIni;
char iniSection[50];

// --- System control objects
byte   TRACE=0x00;
byte   MSW=0x00;
int    vol=10;

// *----------------------------------------------------------------*
// *                  GPIO support processing                       *
// *----------------------------------------------------------------*
// --- gpio object
gpioWrapper* g=nullptr;
char   *gpio_buffer;
void gpiochangePin();

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

// --- rtlfm object
rtlfm*  rtl=nullptr;
char    *rtl_buffer;
#endif


#ifdef Pi4D
DDS*   dds=nullptr;

#endif

// *----------------------------------------------------------------*
// *             FT817 CAT Simulation support                       *
// *----------------------------------------------------------------*
// --- CAT object
void    CATchangeMode();      // Callback when CAT receives a mode change
void    CATchangeFreq();      // Callback when CAT receives a freq change
void    CATchangeStatus();    // Callback when CAT receives a status change
CAT817* cat=nullptr;
byte    FT817;
char    port[80];
long    catbaud=4800;
int     SNR;

void    SSBchangeVOX();
void    changeDDS(float f);
genVFO* vfo=nullptr;

//-**********************************************************************************************************************
//-
//-**********************************************************************************************************************

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
// ======================================================================================================================
static void sighandler(int signum)
{
   (TRACE >= 0x00 ? fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum) : _NOP);
   setWord(&MSW,RUN,false);
}

// *---------------------------------------------------------------------------------------------------------------------
// setPTT(boolean)
// handle PTT variations
// *---------------------------------------------------------------------------------------------------------------------
void setPTT(bool ptt) {

    if (ptt==true) {  //currently receiving now transmitting

#ifdef Pi4D

    // *---------------------------------------------*
    // * if running with DDS it is destroyed while   *
    // * transmitting because LO is not needed       *
    // *---------------------------------------------*
       if (dds!=nullptr) {
          dds->stop();
          delete(dds);
          dds=nullptr;
          (TRACE>=0x01 ? fprintf(stderr,"%s:setPTT(%s) destroyed DDS object\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);
       }
#endif

    // *---------------------------------------------*
    // * Set PTT into transmit mode                  *
    // *---------------------------------------------*
       (TRACE>=0x01 ? fprintf(stderr,"%s:setPTT(%s) operating relay to TX position\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);
       if(g!=nullptr) {g->writePin(GPIO_PTT,1);}
       usleep(10000);

#ifdef Pi4D

       if (usb==nullptr) {
          usb=new genSSB(SSBchangeVOX);  
          usb->TRACE=TRACE;
          usb->setFrequency(f);
          usb->setSoundChannel(CHANNEL);
          usb->setSoundSR(AFRATE);
          usb->setSoundHW(HW);
          usb->start();
          (TRACE>=0x01 ? fprintf(stderr,"%s:setPTT(%s) created USB object\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);
       }

#endif


    // *---------------------------------------------*
    // * Set transceiver into transmit mode          *
    // *---------------------------------------------*
       usb->setPTT(ptt);

    } else {          //currently transmitting, now receiving

       usb->setPTT(ptt);
       usleep(10000);

#ifdef Pi4D
      if (usb!=nullptr) {
         usb->stop();
         delete(usb);
         usb=nullptr;
         (TRACE>=0x01 ? fprintf(stderr,"%s:setPTT(%s) destroyed USB object\n",PROGRAMID,BOOL2CHAR(ptt)) : _NOP);
      }
#endif

       if(g!=nullptr) {g->writePin(GPIO_PTT,0);}
       usleep(10000);


#ifdef Pi4D

       if (dds==nullptr) {
          dds=new DDS(changeDDS);
          dds->TRACE=TRACE;
          dds->gpio=GPIO_DDS;
          dds->power=DDS_MAXLEVEL;
          dds->f=f;
          dds->ppm=1000;
          dds->start(f);
       }
#endif
    }

    setWord(&MSW,PTT,ptt);
    (TRACE>=0x01 ? fprintf(stderr,"%s:setPTT() set PTT as(%s)\n",PROGRAMID,(getWord(MSW,PTT)==true ? "True" : "False")) : _NOP);
    return;
}





//---------------------------------------------------------------------------
// CATchangeFreq()
// CAT Callback when frequency changes
//---------------------------------------------------------------------------
void CATchangeFreq() {

  if (usb->statePTT == true) {
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) request while transmitting, ignored!\n",PROGRAMID,(int)cat->SetFrequency) : _NOP);
     cat->SetFrequency=f;
     return;
  }

  cat->SetFrequency=f;
  (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeFreq() Frequency change is not allowed(%d)\n",PROGRAMID,(int)f) : _NOP);

}
//-----------------------------------------------------------------------------------------------------------
// CATchangeMode
// Validate the new mode is a supported one
// At this point only CW,CWR,USB and LSB are supported
//-----------------------------------------------------------------------------------------------------------
void CATchangeMode() {

  (TRACE>=0x02 ? fprintf(stderr,"%s:CATchangeMode() requested MODE(%d) not supported\n",PROGRAMID,cat->MODE) : _NOP);
  cat->MODE=MUSB;
  return;

}

//------------------------------------------------------------------------------------------------------------
// CATchangeStatus
// Detect which change has been produced and operate accordingly
//------------------------------------------------------------------------------------------------------------
void CATchangeStatus() {

  (TRACE >= 0x03 ? fprintf(stderr,"%s:CATchangeStatus() FT817(%d) cat.FT817(%d)\n",PROGRAMID,FT817,cat->FT817) : _NOP);

  if (getWord(cat->FT817,PTT) != usb->statePTT) {
     (TRACE>=0x02 ? fprintf(stderr,"%s:CATchangeStatus() PTT change request cat.FT817(%d) now is PTT(%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,PTT) ? "true" : "false") : _NOP);
     setPTT(getWord(cat->FT817,PTT));
     setWord(&MSW,PTT,getWord(cat->FT817,PTT));
  }

  if (getWord(cat->FT817,RIT) != getWord(FT817,RIT)) {        // RIT Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() RIT change request cat.FT817(%d) RIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,RIT) ? "true" : "false") : _NOP);
  }

  if (getWord(cat->FT817,LOCK) != getWord(FT817,LOCK)) {      // LOCK Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() LOCK change request cat.FT817(%d) LOCK changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,LOCK) ? "true" : "false") : _NOP);
  }

  if (getWord(cat->FT817,SPLIT) != getWord(FT817,SPLIT)) {    // SPLIT mode Changed
     (TRACE>=0x01 ? fprintf(stderr,"%s:CATchangeStatus() SPLIT change request cat.FT817(%d) SPLIT changed to %s ignored\n",PROGRAMID,cat->FT817,getWord(cat->FT817,SPLIT) ? "true" : "false") : _NOP);
  }

  if (getWord(cat->FT817,VFO) != getWord(FT817,VFO)) {        // VFO Changed
     (TRACE >=0x01 ? fprintf(stderr,"%s:CATchangeStatus() VFO change request not supported\n",PROGRAMID) : _NOP);
  }
  FT817=cat->FT817;
  return;

}


//--------------------------------------------------------------------------------------------------
// Callback to process SNR signal
//--------------------------------------------------------------------------------------------------
void CATgetRX() {

    cat->RX=cat->snr2code(SNR);

}

void CATgetTX() {
}
// ======================================================================================================================
// SNR upcall change
// ======================================================================================================================
void changeSNR() {

#ifdef OT4D
     SNR=rtl->SNR;
#endif

}

// ======================================================================================================================
// DDS upcall change of frequency
// ======================================================================================================================
#ifdef Pi4D
void changeDDS(float f) {


}

#endif
// ======================================================================================================================
// VOX upcall signal
// ======================================================================================================================
void SSBchangeVOX() {

  (TRACE>=0x02 ? fprintf(stderr,"%s:SSBchangeVOX() received upcall from genSSB object state(%s)\n",PROGRAMID,BOOL2CHAR(usb->stateVOX)) : _NOP);
  setPTT(usb->stateVOX);


}
// ======================================================================================================================
// VOX upcall signal
// ======================================================================================================================
void gpiochangePin(int pin,int state) {


  (TRACE>=0x02 ? fprintf(stderr,"%s:gpiochangePin() received upcall from gpioWrapper object state pin(%d) state(%d)\n",PROGRAMID,pin,state) : _NOP);
  if (pin==GPIO_AUX) {
     (state==1 ? setPTT(false) : setPTT(true));
     (TRACE >=0x01 ? fprintf(stderr,"%s:gpiochangePin() manual PTT operation thru AUX button pin(%d) value(%d)\n",PROGRAMID,pin,state) : _NOP);
  }
  
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
  TRACE=ini_getl("OT4D","TRACE",2,inifile);
  vol=ini_getl("OT4D","VOL",10,inifile);
  nIni=ini_gets("OT4D", "PORT", "/tmp/ttyv0", port, sizearray(port), inifile);

  (TRACE>=0x02 ? fprintf(stderr,"%s:main()   FREQ=%g\n",PROGRAMID,f) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main()  TRACE=%d\n",PROGRAMID,TRACE) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main()    VOL=%d\n",PROGRAMID,vol) : _NOP);
  (TRACE>=0x02 ? fprintf(stderr,"%s:main()   PORT=%s\n",PROGRAMID,port) : _NOP);
#endif

// --- memory areas

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize memory areas\n",PROGRAMID) : _NOP);
  gpio_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));
  usb_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));

#ifdef OT4D
  rtl_buffer=(char*)malloc(RTLSIZE*sizeof(unsigned char));
#endif

//---------------------------------------------------------------------------------
// arg_parse (parameters override previous configuration)
//---------------------------------------------------------------------------------
   while(1)
	{
	int ax = getopt(argc, argv, "p:f:t:v:h");
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
	     (TRACE>=0x01 ? fprintf(stderr,"%s: argument port(%s)\n",PROGRAMID,port) : _NOP);
	     break;
	case 'f': // Frequency
	     f = atof(optarg);
  	     (TRACE>=0x01 ? fprintf(stderr,"%s: argument frequency(%10f)\n",PROGRAMID,f) : _NOP);
	     break;
	case 'v': // SampleRate (Only needeed in IQ mode)
	     vol = atoi(optarg);
	     (TRACE>=0x01 ? fprintf(stderr,"%s: argument volume(%d)\n",PROGRAMID,vol) : _NOP);
             break;
	case 't': // SampleRate (Only needeed in IQ mode)
	     TRACE = atoi(optarg);
	     (TRACE>=0x01 ? fprintf(stderr,"%s: argument tracelevel(%d)\n",PROGRAMID,TRACE) : _NOP);
             break;
	case -1:
             break;
	case '?':
	     if (isprint(optopt) )
 	     {
 	        (TRACE>=0x00 ? fprintf(stderr, "%s:arg_parse() unknown option `-%c'.\n",PROGRAMID,optopt) : _NOP);
 	     } 	else {
		(TRACE>=0x00 ?fprintf(stderr, "%s:arg_parse() unknown option character `\\x%x'.\n",PROGRAMID,optopt) : _NOP);
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


// --- ALSA loopback support 

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() initialize ALSA parameters\n",PROGRAMID) : _NOP);
  HW=(char*)malloc(16*sizeof(unsigned char));
  sprintf(HW,SOUNDHW);

// --- gpio Wrapper creation

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize gpio Wrapper\n",PROGRAMID) : _NOP);
  g=new gpioWrapper(gpiochangePin);
  g->TRACE=TRACE;

  if (g->setPin(GPIO_PTT,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
     (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_PTT) : _NOP);
     exit(16);
  }
  if (g->setPin(GPIO_PA,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
     (TRACE>=0x0 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_PA) : _NOP);
     exit(16);
  }
  if (g->setPin(GPIO_AUX,GPIO_IN,GPIO_PUP,GPIO_NLP) == -1) {
     (TRACE>=0x0 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_AUX) : _NOP);
     exit(16);
  }
  if (g->setPin(GPIO_KEYER,GPIO_IN,GPIO_PUP,GPIO_NLP) == -1) {
     (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_KEYER) : _NOP);
     exit(16);
  }

  if (g->setPin(GPIO_COOLER,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
     (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_KEYER) : _NOP);
     exit(16);
  }

  if (g->start() == -1) {
     (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to start gpioWrapper object\n",PROGRAMID) : _NOP);
     exit(8);
  }

  usleep(100000);

  // *---------------------------------------------*
  // * Set cooler ON mode                          *
  // *---------------------------------------------*
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() operating relay to cooler activation\n",PROGRAMID) : _NOP);
  if(g!=nullptr) {g->writePin(GPIO_COOLER,1);}
  usleep(10000);



#ifdef OT4D
// --- define rtl-sdr handling object
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize RTL-SDR controller interface\n",PROGRAMID) : _NOP);
  rtl=new rtlfm();  
  rtl->TRACE=TRACE;
  rtl->setMode(MUSB);
  rtl->setVol(vol);  
  rtl->setFrequency(f);
  rtl->changeSNR=changeSNR;
  rtl->start();

#endif

#ifdef Pi4D
// --- define DDS 
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize RPITX dds controller interface\n",PROGRAMID) : _NOP);
  dds=new DDS(changeDDS);
  dds->TRACE=TRACE;
  dds->gpio=GPIO_DDS;
  dds->power=DDS_MAXLEVEL;
  dds->f=f;
  dds->ppm=1000;
  dds->start(f);
#endif

// --- USB generator 
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize SSB generator interface\n",PROGRAMID) : _NOP);
  usb=new genSSB(SSBchangeVOX);  
  usb->TRACE=TRACE;
  usb->setFrequency(f);
  usb->setSoundChannel(CHANNEL);
  usb->setSoundSR(AFRATE);
  usb->setSoundHW(HW);
  usb->start();


// --- creation of CAT object

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize CAT controller interface\n",PROGRAMID) : _NOP);
  FT817=0x00;
  cat=new CAT817(CATchangeFreq,CATchangeStatus,CATchangeMode,CATgetRX,CATgetTX);
  cat->FT817=FT817;
  cat->POWER=DDS_MAXLEVEL;
  cat->SetFrequency=f;
  cat->MODE=MUSB;
  cat->TRACE=TRACE;
  cat->open(port,catbaud);
  cat->getRX=CATgetRX;

  setWord(&cat->FT817,AGC,false);
  setWord(&cat->FT817,PTT,getWord(MSW,PTT));

  vfo=new genVFO(NULL);
  vfo->TRACE=TRACE;
  vfo->FT817=FT817;
  vfo->MODE=cat->MODE;
  vfo->set(VFOA,f);
  vfo->set(VFOB,f);
  vfo->setSplit(false);
  vfo->setRIT(VFOA,false);
  vfo->setRIT(VFOB,false);

  vfo->vfo=VFOA;
  setWord(&cat->FT817,VFO,VFOA);
  

// -- establish loop condition
  
  
  setWord(&MSW,RUN,true);
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() start operation\n",PROGRAMID) : _NOP);

// --- Cycle the PTT to check the interface  

  setPTT(true);
  sleep(ONESEC);
  setPTT(false);
  

//*-------------------------------------------------------------------------------------------*
//*                                 MAIN PROGRAM LOOP                                         *
//*-------------------------------------------------------------------------------------------*
  while(getWord(MSW,RUN)==true)
 
  {
    //*---------------------------------------------*
    //*           Process CAT commands              *
    //*---------------------------------------------*
    cat->get();

    //*---------------------------------------------*
    //* Process commands from rtl-sdr receiver      *
    //*---------------------------------------------*
#ifdef OT4D
    if (getWord(rtl->MSW,RUN)==true) {

    int rtl_read=rtl->readpipe(rtl_buffer,BUFSIZE);
        if (rtl_read>0) {
           rtl_buffer[rtl_read]=0x00;
           (TRACE>=0x01 ? fprintf(stderr,"%s",(char*)rtl_buffer) : _NOP);
        }
    }

    if (getWord(rtl->MSW,RUN)==true) {

    int rtl_read=rtl->readpipe(rtl_buffer,BUFSIZE);
        if (rtl_read>0) {
           rtl_buffer[rtl_read]=0x00;
           (TRACE>=0x02 ? fprintf(stderr,"%s",(char*)rtl_buffer) : _NOP);
        }
    }
#endif

    //*---------------------------------------------*
    //* Process signals and messages from GPIO      *
    //*---------------------------------------------*
    if (getWord(g->MSW,RUN)==true) {
    int gpio_read=g->readpipe(gpio_buffer,BUFSIZE);
        if (gpio_read>0) {
           gpio_buffer[gpio_read]=0x00;
           (TRACE>=0x02 ? fprintf(stderr,"%s",(char*)gpio_buffer) : _NOP);
        }
    }

  }

// --- Normal termination kills the child first and wait for its termination
#ifdef OT4D
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


  // *---------------------------------------------*
  // * Set cooler ON mode                          *
  // *---------------------------------------------*
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() operating relay to cooler de-activation\n",PROGRAMID) : _NOP);
  if(g!=nullptr) {g->writePin(GPIO_COOLER,0);}
  usleep(10000);


// --- Stop all threads and child processes 

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() stopping operations\n",PROGRAMID) : _NOP);

  g->stop();
  cat->close();
  usb->stop();

#ifdef OT4D
  rtl->stop();
#endif

#ifdef Pi4D
  dds->stop();
#endif
}
