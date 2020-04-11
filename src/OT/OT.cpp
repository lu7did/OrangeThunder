/* 
 * OT.cpp
 * Raspberry Pi based USB experimental SSB Generator for digital modes (mainly WSPR and FT8)
 * Experimental version largely modelled after rtl_fm  and librpitx
 * This program tries to mimic the behaviour of simple QRP SSB transceivers
 * SSB generation by the librpitx library isn´t linear but not much different from 
 * highly compressed HF SSB voice signals, the TAPR amplifier isn´t much linear either.
 * So the transceiver is specially suited to PSK and FSK modulated modes such as WSPR,FT8, SSTV, CW or PSK31.
 *---------------------------------------------------------------------
 * This program is a sister project from a Raspberry Pi used to control
 * a Pixie transceiver hardware.
 * Project at http://www.github.com/lu7did/PixiePi
 *---------------------------------------------------------------------
 *  
 *
 * Created by Pedro E. Colla (lu7did@gmail.com)
 * Code excerpts from several packages:
 *    rtl_fm.c from Steve Markgraf <steve@steve-m.de> and others 
 *    Adafruit's python code for CharLCDPlate
 *    tune.cpp from rpitx package by Evariste Courjaud F5OEO
 *    sendiq.cpp from rpitx package (also) by Evariste Coujaud (F5EOE)
 *    wiringPi library (git clone git://git.drogon.net/wiringPi)
 *    iambic-keyer (https://github.com/n1gp/iambic-keyer)
 *    log.c logging facility by  rxi https://github.com/rxi/log.c
 *    minIni configuration management package by Compuphase https://github.com/compuphase/minIni/tree/master/dev
 *    tinyalsa https://github.com/tinyalsa/tinyalsa
 *    PJ_RPI  GPIO low level handler git clone git://github.com/Pieter-Jan/PJ_RPI.git
 * Also libraries
 *    librpitx by  Evariste Courjaud (F5EOE)
 *    libcsdr by Karol Simonyi (HA7ILM) https://github.com/compuphase/minIni/tree/master/dev
 *
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#define MAX_SAMPLERATE 200000
#define BUFFERSIZE      96000
#define IQBURST          4000
#define VOX_TIMER        20

#define GPIO_PTT         12
#define GPIO_KEYER       16
#define GPIO_PULSE       20
#define GPIO_PA          21


#include <unistd.h>
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <cstdlib>      // for std::rand() and std::srand()
#include <ctime>        // for std::time()
#include <iostream>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits.h>
#include "/home/pi/PixiePi/src/lib/SSB.h" 
#include "/home/pi/PixiePi/src/lib/CAT817.h" 
#include "/home/pi/PixiePi/src/lib/VFOSystem.h" 
#include "/home/pi/PixiePi/src/pixie/pixie.h" 
#include "/home/pi/librpitx/src/librpitx.h"
#include "/home/pi/PixiePi/src/lib/RPI.h" 
// #include "/home/pi/PixiePi/src/lib/DDS.h"
#include "/home/pi/PixiePi/src/minIni/minIni.h"
#include "/home/pi/OrangeThunder/src/lib/rtlfm.h"


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x)

#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))

//-------------------- GLOBAL VARIABLES ----------------------------

const char   *PROGRAMID="OT";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
enum  {typeiq_i16,typeiq_u8,typeiq_float,typeiq_double,typeiq_carrier};

const char   *CFGFILE="OT.cfg";
typedef unsigned char byte;
typedef bool boolean;

// ****************************************************************************************************
// --- DDS & I/Q Generator
// ****************************************************************************************************
iqdmasync*  iqtest=nullptr;
rtlfm*      rtl=nullptr;

#define     VFO_LOWER_LIMIT   14000000
#define     VFO_HIGHER_LIMIT  14299000

float       SetFrequency=14074000;
float       SampleRate=6000;

// --- Define minIni related parameters

char        inifile[80];
char        iniStr[100];
long        nIni;
int         sIni;
int         kIni;
char        iniSection[50];

// --- Transceiver control structure
long int TVOX=0;
byte MSW=0;

//byte trace=0x00;
byte TRACE=0x02;

// --- CAT object

void CATchangeMode();
void CATchangeFreq();
void CATchangeStatus();

VFOSystem *vfo;
CAT817* cat;
byte FT817;
long int  bButtonAnt=0;

char  port[80];
long  catbaud=CATBAUD;

int   ax;
int   anyargs = 1;

//bool loop_mode_flag=false;

char  FileName[80];

int   Harmonic=1;
int   InputType=typeiq_i16;
int   Decimation=1;

int   m=1;
int   SR=48000;
int   FifoSize=IQBURST*4;

// --- SSB voice generation objects

float  agc_rate=0.25;
float  agc_reference=1.0;
float  agc_max_gain=5.0;
float  agc_current_gain=1.0;

SSB*   usb;
float* Ibuffer;
float* Qbuffer;

float  gain=1.0;

int    numSamples=0;
int    numSamplesLow=0;
int    exitrecurse=0;
int    bufferLengthInBytes;
short  *buffer_i16;

// --- IQ Buffer

std::complex<float> CIQBuffer[IQBURST];	
//--------------------------------------------------------------------------------------------------
// map_peripheral
// Exposes the physical address defined in the passed structure using mmap on /dev/mem
//--------------------------------------------------------------------------------------------------
int map_peripheral(struct bcm2835_peripheral *p)
{
   // Open /dev/mem
   if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("Failed to open /dev/mem, try checking permissions.\n");
      return -1;
   }
 
   p->map = mmap(
      NULL,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,      // File descriptor to physical memory virtual file '/dev/mem'
      p->addr_p       // Address in physical map that we want this memory block to expose
   );
 
   if (p->map == MAP_FAILED) {
        perror("mmap");
        return -1;
   }
 
   p->addr = (volatile unsigned int *)p->map;
 
   return 0;
}
//--------------------------------------------------------------------------------------------------
// unmap_peripheral
// release resources
//--------------------------------------------------------------------------------------------------
void unmap_peripheral(struct bcm2835_peripheral *p) {
 
    munmap(p->map, BLOCK_SIZE);
    close(p->mem_fd);
}

//--------------------------------------------------------------------------------------------------
// setGPIO
// output status of a given GPIO pin
//--------------------------------------------------------------------------------------------------
void setGPIO(int pin,bool v) {

// ---  acquire resources
 return;

 if(map_peripheral(&gpio) == -1) 
  {
    fprintf(stderr,"Failed to map the physical GPIO registers into the virtual memory space.\n");
    return ;
  }
// --- Clear pin definition and then set as output

  INP_GPIO(pin);
  OUT_GPIO(pin);

// --- Map result to pin
 
  if (v==true) {
    GPIO_SET = 1 << pin;  // Set as high
  } else {
    GPIO_CLR = 1 << pin;  // Set as low
  }

// --- release resources

  unmap_peripheral(&gpio);
  return; 
}
//--------------------------------------------------------------------------------------------------
// getGPIO
// get value of a GPIO pin
//--------------------------------------------------------------------------------------------------
long int getGPIO(int pin) {

// ---  acquire resources

 if(map_peripheral(&gpio) == -1) 
  {
    fprintf(stderr,"Failed to map the physical GPIO registers into the virtual memory space.\n");
    return 0;
  }
// --- Clear pin definition and then set as output

  INP_GPIO(pin);
  int v=GPIO_READ(pin);

// --- release resources

  unmap_peripheral(&gpio);
  return (long int) v;
}
//--------------------------------------------------------------------------------------------
// set_PTT
// Manage the PTT of the transceiver (can be used from the keyer or elsewhere
//--------------------------------------------------------------------------------------------
void setPTT(bool statePTT) {

//---------------------------------*
//          PTT Activated          *
//---------------------------------*
    if (statePTT==true) {

//--- Turn PTT on

        setWord(&cat->FT817,PTT,true);
        setWord(&MSW,PTT,true);
        setGPIO(GPIO_PTT,true);

        if (iqtest == nullptr) {
           iqtest=new iqdmasync(SetFrequency,SampleRate,14,FifoSize,MODE_IQ);
           iqtest->SetPLLMasterLoop(3,4,0);
           cat->SetFrequency=SetFrequency;
           usleep(10000);
           fprintf(stderr,"%s:setPTT(false) removing I/Q processor\n",PROGRAMID);
        }
        fprintf(stderr,"%s:setPTT(true) set\n",PROGRAMID);
        return;

    } 

//---------------------------------*
//          PTT Inactivated        *
//---------------------------------*

    if (iqtest != nullptr) {
       iqtest->stop();
       delete(iqtest);
       iqtest=nullptr;
       usleep(10000);
       fprintf(stderr,"%s:setPTT(false) removing I/Q processor\n",PROGRAMID);
    }

    setWord(&cat->FT817,PTT,false);
    setWord(&MSW,PTT,false);
    setGPIO(GPIO_PTT,false);

    if (rtl == nullptr) {
       rtl=new rtlfm();
       rtl->setFrequency(SetFrequency);
       rtl->setMode(MUSB);
       rtl->TRACE=TRACE;
       rtl->start();
       fprintf(stderr,"%s:setPTT(false) starting rtl-sdr initialized\n",PROGRAMID);
    }
    fprintf(stderr,"%s:setPTT(false) set\n",PROGRAMID);
    return;

}
//--------------------------------------------------------------------------------------------------
// checkAux
// check the aux button
//--------------------------------------------------------------------------------------------------
void checkAux() {

   long int bButton=getGPIO(GPIO_PULSE);

   if (bButton!=bButtonAnt) {
      bButtonAnt=bButton;
      if (getWord(MSW,PTT)==false) {
         if (bButton == 0) {
            fprintf(stderr,"%s:checkAux() AUX Pin Pressed (%ld)->(%ld)\n",PROGRAMID,bButtonAnt,bButton);
            setWord(&MSW,TUNE,true);
            setPTT(true);
        } else {
            fprintf(stderr,"%s:checkAux() AUX Pin Released (%ld)->(%ld)\n",PROGRAMID,bButtonAnt,bButton);
            setPTT(false);
        }
     } else {
       fprintf(stderr,"%s AUX Pin activity ignored while transmitting\n",PROGRAMID);
     }
  } 
     
}

//---------------------------------------------------------------------------
// CATchangeFreq()
// CAT Callback when frequency changes
//---------------------------------------------------------------------------
void CATchangeFreq() {

  //fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) SetFrequency(%d)\n",PROGRAMID,(int)cat->SetFrequency,(int)SetFrequency);
  if ((cat->SetFrequency<VFO_LOWER_LIMIT) || (cat->SetFrequency>VFO_HIGHER_LIMIT)) {
     fprintf(stderr,"%s::CATchangeFreq() cat.SetFrequency(%d) out of band is rejected\n",PROGRAMID,(int)cat->SetFrequency);
     cat->SetFrequency=SetFrequency;
     vfo->set(vfo->vfoAB,(long int)SetFrequency);
     return;
  }


  if (getWord(MSW,PTT) == true) {
     fprintf(stderr,"%s:CATchangeFreq() cat.SetFrequency(%d) request while transmitting, ignored!\n",PROGRAMID,(int)cat->SetFrequency);
     cat->SetFrequency=SetFrequency;
     vfo->set(vfo->vfoAB,(long int)SetFrequency);
     return;

// --- DEAD CODE
     if (iqtest != nullptr) {
        iqtest->clkgpio::disableclk(GPIO04);
        iqtest->clkgpio::SetAdvancedPllMode(true);
        iqtest->clkgpio::SetCenterFrequency(SetFrequency,SampleRate);
        iqtest->clkgpio::SetFrequency(0);
        iqtest->clkgpio::enableclk(GPIO04);
     }
// ---
  }


  SetFrequency=cat->SetFrequency;
  vfo->set(vfo->vfoAB,(long int) SetFrequency);
  if (rtl != nullptr) {
     rtl->setFrequency(SetFrequency);
  }
  fprintf(stderr,"%s:CATchangeFreq() Frequency set to SetFrequency(%d)\n",PROGRAMID,(int)SetFrequency);
}
//-----------------------------------------------------------------------------------------------------------
// CATchangeMode
// Validate the new mode is a supported one
// At this point only CW,CWR,USB and LSB are supported
//-----------------------------------------------------------------------------------------------------------
void CATchangeMode() {

  if (cat->MODE == MUSB || cat->MODE == MLSB || cat->MODE == MAM || cat->MODE == MFM || cat->MODE == MWFM) {
     rtl->setMode(cat->MODE);
     fprintf(stderr,"%s:CATchangeMode() mode change to MODE(%d) accepted\n",PROGRAMID,cat->MODE);
     return;
  }

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

  if (getWord(cat->FT817,PTT) != getWord(FT817,PTT)) {        // PTT Changed
     fprintf(stderr,"%s:CATchangeStatus() PTT change request cat.FT817(%d) now is PTT(%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,PTT) ? "true" : "false");
     setPTT(getWord(cat->FT817,PTT));
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
     setWord(&FT817,VFO,getWord(cat->FT817,VFO));
     (getWord(cat->FT817,VFO)==false ? vfo->vfoAB = VFOA : vfo->vfoAB = VFOB);
     cat->SetFrequency = (float) vfo->get(vfo->vfoAB);
     fprintf(stderr,"%s:CATchangeStatus() VFO change request cat.FT817(%d) VFO changed to (%s)\n",PROGRAMID,cat->FT817,getWord(cat->FT817,VFO) ? "VFO A" : "VFO B");
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
//--------------------------------------------------------------------------------------------------

// timer_exec 
// timer management
//--------------------------------------------------------------------------------------------------
void timer_exec()
{
  if (TVOX!=0) {
     TVOX--;
     if(TVOX==0) {
       //fprintf(stderr,"%s::timer_exec() VOX timer expiration event\n",PROGRAMID);
       setWord(&MSW,VOX,true);
     }
  }
}

//---------------------------------------------------------------------------
// Timer handler function
//---------------------------------------------------------------------------
void timer_start(std::function<void(void)> func, unsigned int interval)
{
  std::thread([func, interval]()
  {
    while (getWord(MSW,RUN)==true)
    {
      auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
      func();
      std::this_thread::sleep_until(x);
    }
  }).detach();
}

//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\
Usage: [-i File Input][-s Samplerate][-l] [-f Frequency] [-h Harmonic number] \n\
-i            path to File Input \n\
-s            SampleRate 10000-250000 \n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-c            configuration file definition\n\
-a            activate VOX\n\
-h            Use harmonic number n\n\
-?            help (this help).\n\
\n",PROGRAMID,PROG_VERSION,PROG_BUILD);


} /* end function print_usage */
//---------------------------------------------------------------------------------
// terminate
//---------------------------------------------------------------------------------

static void terminate(int num)
{
    setWord(&MSW,RUN,false);
    fprintf(stderr,"%s:terminate() Caught TERM signal(%x) - Terminating \n",PROGRAMID,num);
    if (exitrecurse > 0) {
       fprintf(stderr,"%s:terminate() Recursive trap - Force termination \n",PROGRAMID);
       exit(16);
    }
    exitrecurse++;

// --- Terminate rtl-sdr dongle

    //fprintf(stderr, "%s: Terminating rtl-sdr dongle!\n",PROGRAMID);
    //do_exit = 1;
    //rtlsdr_cancel_async(dongle.dev);



}
//---------------------------------------------------------------------------------
// arg_parse
//---------------------------------------------------------------------------------
void arg_parse(int argc, char* argv[]) {

	while(1)
	{
		ax = getopt(argc, argv, "i:f:s:p:h:c:at:");
		if(ax == -1) 
		{
			if(anyargs) break;
			else ax='h'; //print usage and exit
		}
		anyargs = 1;

		switch(ax)
		{
		case 'i': // File name
                        sprintf(FileName,optarg);
			//FileName = optarg;
			fprintf(stderr,"%s:arg_parse() Filename(%s)\n",PROGRAMID,FileName);
			break;
		case 'c': // INI file
                        strcpy(inifile,optarg);
			fprintf(stderr,"%s:arg_parse() Configuration file (%s)\n",PROGRAMID,inifile);
			break;
		case 'a': // loop mode
			usb->agc.active=true;
			fprintf(stderr,"%s:arg_parse() AGC activated\n",PROGRAMID);
			break;
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			//cat->SetFrequency=SetFrequency;
			fprintf(stderr,"%s:arg_parse() Frequency(%10f)\n",PROGRAMID,SetFrequency);
			break;
                case 'p': //serial port
                        sprintf(port,optarg);
                        fprintf(stderr,"%s:arg_parse() Serial Port(%s)\n",PROGRAMID, port);
                        break;
		case 's': // SampleRate (Only needeed in IQ mode)
			SampleRate = atoi(optarg);
			fprintf(stderr,"%s:arg_parse() SampleRate(%10f)\n",PROGRAMID,SampleRate);
			if(SampleRate>MAX_SAMPLERATE) 
			{
				for(int i=2;i<12;i++) //Max 10 times samplerate
				{
					if(SampleRate/i<MAX_SAMPLERATE) 
					{
						SampleRate=SampleRate/i;
						Decimation=i;
						break;
					}
				}
				if(Decimation==1)
				{
					 fprintf(stderr,"%s:arg_parse() SampleRate too high : >%d sample/s",PROGRAMID,10*MAX_SAMPLERATE);
					 exit(1);
				} 
				else
				{
					fprintf(stderr,"%s:arg_parse() Warning samplerate too high, decimation by %d will be performed",PROGRAMID,Decimation);	 
				}
			};
			break;
		case 'h': // help
			Harmonic=atoi(optarg);
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 			   fprintf(stderr, "%s:arg_parse() unknown option `-%c'.\n",PROGRAMID,optopt);
 			}
			else
			{
				fprintf(stderr, "%s:arg_parse() unknown option character `\\x%x'.\n",PROGRAMID,optopt);
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

}
//---------------------------------------------------------------------------------
// main 
//---------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

        fprintf(stderr,"%s %s [%s]\n",PROGRAMID,PROG_VERSION,PROG_BUILD);

        InputType=typeiq_float;
        sprintf(port,"/tmp/ttyv0");
        sprintf(FileName,"-");
        timer_start(timer_exec,100);

        strcpy(inifile,CFGFILE);

        setWord(&MSW,RUN,true);
        setWord(&MSW,VOX,false);

        fprintf(stderr,"%s:main(): GPIO low level controller\n",PROGRAMID); 
        setGPIO(GPIO_PTT,false);


float  agc_rate=0.25;
float  agc_reference=1.0;
float  agc_max_gain=5.0;
float  agc_current_gain=1.0;



//--------------------------------------------------------------------------------------------------
// SSB (USB) controller generation
//--------------------------------------------------------------------------------------------------
float   gain=1.0;

        usb=new SSB();
        usb->agc.reference=1.0;
	usb->agc.max_gain=2.0;
	usb->agc.rate=0.25;
        usb->agc.active=false;
	usb->agc.gain=&gain;

        fprintf(stderr,"%s:main(): SSB controller generation\n",PROGRAMID); 

//--------------------------------------------------------------------------------------------------
//      Argument parsting
//      First pass, all parms pretty much ignored except for configuration file
//--------------------------------------------------------------------------------------------------
        //arg_parse(argc,argv);


//--------------------------------------------------------------------------------------------------
//      Process configuration file
//--------------------------------------------------------------------------------------------------

        //SetFrequency=(float)ini_getl("Pi4D","FREQUENCY",(long int)SetFrequency,inifile);
        //cat->SetFrequency=SetFrequency;

        SampleRate=(float)ini_getl("OT","SAMPLERATE",(long int)SampleRate,inifile);

        agc_rate=(float)(ini_getl("OT","AGC_RATE",(long int)(agc_rate*100),inifile))/100.0;
        agc_reference=(float)(ini_getl("OT","AGC_REF",(long int)(agc_reference*100),inifile))/100.0;
        agc_max_gain=(float)(ini_getl("OT","AGC_MAX_GAIN",(long int)(agc_max_gain*100),inifile))/100.0;


        nIni=ini_gets("OT", "PORT", "/tmp/ttyv0", port, sizearray(port), inifile);
        nIni=ini_gets("OT", "INPUT", "/dev/stdin", FileName, sizearray(FileName), inifile);
        catbaud=(long int)ini_getl("OT","BAUD",CATBAUD,inifile);

        fprintf(stderr,"%s:main():main() INI file processed\n",PROGRAMID); 
        fprintf(stderr,"%s:main():main() INI SR(%g) AGCRate(%g) AGCRef(%g) AGCMax(%g)\n",PROGRAMID,SampleRate,agc_rate,agc_reference,agc_max_gain);
        fprintf(stderr,"%s:main():main() INI Port(%s) Baud(%li) Input(%s)\n",PROGRAMID,port,catbaud,FileName);

//--------------------------------------------------------------------------------------------------
//      Argument parsting
//      Second pass, this is to override all configuration coming from a file
//--------------------------------------------------------------------------------------------------
        arg_parse(argc,argv);

//--------------------------------------------------------------------------------------------------
// Setup trap handling
//--------------------------------------------------------------------------------------------------
        fprintf(stderr,"%s:main(): Trap handler initialization\n",PROGRAMID);

	for (int i = 0; i < 64; i++) {
           struct sigaction sa;
           std::memset(&sa, 0, sizeof(sa));
           sa.sa_handler = terminate;
           sigaction(i, &sa, NULL);
        }


//--------------------------------------------------------------------------------------------------
// Setup VFO Object
//--------------------------------------------------------------------------------------------------
        fprintf(stderr,"%s:main() Creating VFO objecdt\n",PROGRAMID);
        vfo=new VFOSystem(NULL,NULL,NULL,NULL);

        vfo->set(VFOA,(long int)SetFrequency);
        vfo->set(VFOB,(long int)SetFrequency);
        vfo->setVFO(VFOA);
        vfo->setVFOLimit(VFOA,(long int)VFO_LOWER_LIMIT,(long int)VFO_HIGHER_LIMIT);
        vfo->setVFOLimit(VFOB,(long int)VFO_LOWER_LIMIT,(long int)VFO_HIGHER_LIMIT);

//--------------------------------------------------------------------------------------------------
// Setup CAT object
//--------------------------------------------------------------------------------------------------

        fprintf(stderr,"%s:main(): Creating CAT object\n",PROGRAMID); 

        cat=new CAT817(CATchangeFreq,CATchangeStatus,CATchangeMode,CATgetRX,CATgetTX);

        FT817=0x00;
        cat->FT817=FT817;
        cat->POWER=7;
        cat->SetFrequency=SetFrequency;
        cat->MODE=MUSB;
        cat->TRACE=TRACE;

        cat->open(port,catbaud);
        setWord(&cat->FT817,AGC,false);
        setWord(&cat->FT817,PTT,false);

// ---
// Standard input definition
// ---

        fprintf(stderr,"%s:main(): Standard input definition\n",PROGRAMID);

	FILE *iqfile=NULL;
	if(strcmp(FileName,"-")==0) {
   	  iqfile=fopen("/dev/stdin","rb");
          fprintf(stderr,"%s Sound data from Standard Input (%s)\n",PROGRAMID,FileName);
	} else {
	  iqfile=fopen(FileName	,"rb");
          fprintf(stderr,"%s Sound data from file(%s)\n",PROGRAMID,FileName);

        }

	if (iqfile==NULL) 
	{
   	   printf("%s: input file issue\n",PROGRAMID);
	   exit(0);
	}

        fprintf(stderr,"%s: Input from %s\n",PROGRAMID,FileName);

// ---
// generate DSP buffer areas
// ---
        fprintf(stderr,"%s:main(): Memory buffer creation\n",PROGRAMID);
        buffer_i16 =(short*)malloc(SR*sizeof(short)*2);
        Ibuffer =(float*)malloc(IQBURST*sizeof(short)*2);
        Qbuffer =(float*)malloc(IQBURST*sizeof(short)*2);

        fprintf(stderr,"%s:main(): FIFO buffer creation\n",PROGRAMID); 
	std::complex<float> CIQBuffer[IQBURST];	
        int numBytesRead=0;

        setPTT(false);

        fprintf(stderr,"%s:main() Starting operations TRACE[%d] CAT[%s]\n",PROGRAMID,TRACE,(cat->active==true ? "True" : "False"));
        setWord(&MSW,RUN,true);

        float voxmin=usb->agc.max_gain;
        float voxmax=0.0;
        float voxlvl=voxmin;
        char* bufferr;
        bufferr=(char*)malloc(2048*sizeof(unsigned char));

// ==================================================================================================================================
//                                               MAIN LOOP
// ==================================================================================================================================
	while(getWord(MSW,RUN)==true)
	{
			int CplxSampleNumber=0;
                        if (rtl!=nullptr) {
                           int rerr=rtl->readpipe(bufferr,1024);
                           bufferr[rerr]=0x00;
                           fprintf(stderr,"%s:STDERR->%s",PROGRAMID,(char*)bufferr);
                        }
  			cat->get();
			checkAux();
			switch(InputType)
			{
				case typeiq_float:
				{
					int nbread=fread(buffer_i16,sizeof(short),1024,iqfile);
					if(nbread>0)
					{
					  //fprintf(stderr,"%s:loop() PTT(%d) read(%d) frames\n",PROGRAMID,getWord(MSW,PTT),nbread);
					  int numSamplesLow=usb->generate(buffer_i16,nbread,Ibuffer,Qbuffer);
					  if (getWord(MSW,PTT)==true) {
					     for(int i=0;i<numSamplesLow;i++)
					     {
 				                CIQBuffer[CplxSampleNumber++]=std::complex<float>(Ibuffer[i],Qbuffer[i]);
					      }
					  } else {
					     numSamplesLow=1024/usb->decimation_factor;
					     for(int i=0;i<numSamplesLow;i++)
					     {
 				               CIQBuffer[CplxSampleNumber++]=std::complex<float>(0.0,0.0);
					     }
 					  }
					} else {
					  printf("%s: End of file\n",PROGRAMID);
                                          setWord(&MSW,RUN,false);
					}
				}
				break;	
		}
		if (getWord(MSW,PTT)==true) {
		   iqtest->SetIQSamples(CIQBuffer,CplxSampleNumber,Harmonic);
                } else {
                   usleep(100000);
                }

// ---
// VOX controller (only if enabled)
// ---
          if (usb->agc.active==true) {

                if (gain>voxmax) {
		   voxmax=gain;
		   voxlvl=voxmax*0.90;
                }

		if (gain<voxmin) {
		   voxmin=gain;
                }
 
		if (gain<=voxlvl) {
  		   TVOX=VOX_TIMER;
		   setWord(&MSW,VOX,false);

		   if (getWord(MSW,PTT)==false) {
                      fprintf(stderr,"%s:loop() VOX is activated\n",PROGRAMID);
		      setPTT(true);
		      setWord(&MSW,PTT,true);

		   }
		}

		if (getWord(MSW,VOX)==true) {
		   setWord(&MSW,VOX,false);
                   fprintf(stderr,"%s:loop() VOX is turned off\n",PROGRAMID);
		   setPTT(false);
		   setWord(&MSW,PTT,false);

		}
           }
	}


// ==================================================================================================================================
// end of loop  
// ==================================================================================================================================
        fprintf(stderr,"%s: Turning off virtual rig and cooler\n",PROGRAMID);
        setPTT(false);

// ---
// Deactivating 
// ---
        fprintf(stderr,"%s: Removing RF I/O object\n",PROGRAMID);
        if (iqtest!=nullptr) {  
   	   iqtest->stop();
           delete(iqtest);
	}

        iqtest=nullptr;

        rtl->stop();
        fprintf(stderr,"%s: RTL-SDR receiver stopped\n",PROGRAMID);

        setGPIO(KEYER_OUT_GPIO,false);
        setGPIO(COOLER_GPIO,false);


// --- Save configuration file upon exit

        sprintf(iniStr,"%f",SetFrequency);
        nIni = ini_puts("OT","FREQUENCY",iniStr,inifile);

        sprintf(iniStr,"%f",SampleRate);
        nIni = ini_puts("OT","SAMPLERATE",iniStr,inifile);

        sprintf(iniStr,"%f",agc_rate*100.0);
        nIni = ini_puts("OT","AGC_RATE",iniStr,inifile);

        sprintf(iniStr,"%f",agc_reference*100.0);
        nIni = ini_puts("OT","AGC_REF",iniStr,inifile);

        sprintf(iniStr,"%f",agc_max_gain*100.0);
        nIni = ini_puts("OT","AGC_MAX_GAIN",iniStr,inifile);

        sprintf(iniStr,"%s",port);
        nIni = ini_puts("OT","PORT",iniStr,inifile);

        sprintf(iniStr,"%s",FileName);
        nIni = ini_puts("OT","INPUT",iniStr,inifile);

        sprintf(iniStr,"%ld",catbaud);
        nIni = ini_puts("OT","BAUD",iniStr,inifile);

}

