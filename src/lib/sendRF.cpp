/*
 * genRF.cpp
 * Raspberry Pi based USB experimental SSB Generator
 * Experimental version largely modelled after sendiq.cpp by Evariste (F5OEO) as part of the rpitx package
 *---------------------------------------------------------------------
 * This program generates RF either from an I/Q signal (as sendiq does) or can it be switched
 * to just held a carrier to operate as a DDS. Some commands to also change the frequency and the level
 * has been introduced. This program requires a modified librpitx library allowing the ModeIQ attribute to
 * be writtable.
 * Project at http://www.github.com/lu7did/OrangeThunder
 *---------------------------------------------------------------------
 *
 * Created by Pedro E. Colla (lu7did@gmail.com)
 * Code excerpts from several packages:
 *    Adafruit's python code for CharLCDPlate
 *    tune.cpp from rpitx package by Evariste Courjaud F5OEO
 *    sendiq.cpp from rpitx package (also) by Evariste Coujaud (F5EOE)
 *    wiringPi library (git clone git://git.drogon.net/wiringPi)
 *    iambic-keyer (https://github.com/n1gp/iambic-keyer)
 *    log.c logging facility by  rxi https://github.com/rxi/log.c
 *    minIni configuration management package by Compuphase https://github.com/compuphase/minIni/tree/master/dev
 *    tinyalsa https://github.com/tinyalsa/tinyalsa
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
#include <unistd.h>
#include "/home/pi/librpitx/src/librpitx.h"
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include "/home/pi/OrangeThunder/src/OT/OT.h"
#define MAX_SAMPLERATE 200000
#define IQBURST 4000

//bool running=true;

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="sendRF";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


byte     TRACE=0x00;
byte     MSW=0x00;

typedef unsigned char byte;
typedef bool boolean;
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
void print_usage(void)
{

fprintf(stderr,\
"\n%s version %s build(%s)\n\
Usage:\nsendiq [-i File Input][-s Samplerate][-l] [-f Frequency] [-h Harmonic number] \n\
-i            path to File Input \n\
-s            SampleRate 10000-250000 \n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-?            help (this help).\n\
\n",\
PROGRAMID,PROG_VERSION,PROG_BUILD);

} /* end function print_usage */

//---------------------------------------------------------------------------------------------
static void terminate(int num)
{
    fprintf(stderr,"Caught signal - Terminating %x\n",num);
    setWord(&MSW,RUN,false);  //running=false;
    if (getWord(MSW,RETRY)==true) {
       fprintf(stderr,"Re-entry of signal handling, forcing termination signal(%x)\n",num);
       exit(16);
    }
    setWord(&MSW,RETRY,true);
}
//---------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int a;
	int anyargs = 1;
	float SetFrequency=7074000;
	float SampleRate=48000;
	bool loop_mode_flag=false;
	char* FileName=NULL;
	int Harmonic=1;
	enum {typeiq_i16,typeiq_u8,typeiq_float,typeiq_double};
	int InputType=typeiq_float;
	int Decimation=1;
      float driveDDS=1.0;
      float f=14074000.0;
        bool fdds=false;
	while(1)
	{
		a = getopt(argc, argv, "i:f:s:d");
	
		if(a == -1) 
		{
			if(anyargs) break;
			else a='h'; //print usage and exit
		}
		anyargs = 1;	

		switch(a)
		{
		case 'i': // File name
			FileName = optarg;
			break;
                case 'l': // File name
			driveDDS=atof(optarg);
                        if(driveDDS<0) {driveDDS=0.0;}
                        if(driveDDS>7) {driveDDS=7.0;}
			break;
                case 'd': // File name
			fdds = true;
			break;
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			break;
		case 's': // SampleRate (Only needeed in IQ mode)
			SampleRate = atoi(optarg);
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
					 fprintf(stderr,"%s:main() args: SampleRate too high : >%d sample/s",PROGRAMID,10*MAX_SAMPLERATE);
					 exit(1);
				} 
				else
				{
					fprintf(stderr,"%s:main() args: Warning samplerate too high, decimation by %d will be performed",PROGRAMID,Decimation);	 
				}
			};
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 				fprintf(stderr, "%s:main() args: unknown option `-%c'.\n",PROGRAMID, optopt);
 			}
			else
			{
				fprintf(stderr, "%s:main() args: unknown option character `\\x%x'.\n",PROGRAMID, optopt);
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

	if(FileName==NULL) {fprintf(stderr,"%s:main() Need an input\n",PROGRAMID);exit(1);}
	
	 for (int i = 0; i < 64; i++) {
        struct sigaction sa;

        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
        }

	FILE *iqfile=NULL;
	if(strcmp(FileName,"-")==0) {
		iqfile=fopen("/dev/stdin","rb");
	} else {
		iqfile=fopen(FileName	,"rb");
        } 
	if (iqfile==NULL) { 
	   printf("%s:main() *** ERROR *** input file issue\n",PROGRAMID);
	   exit(0);
	}

	
	int SR=48000;
	int FifoSize=IQBURST*4;
	iqdmasync iqtest(SetFrequency,SampleRate,14,FifoSize,MODE_IQ);
	iqtest.SetPLLMasterLoop(3,4,0);

        if (fdds==true) {  //if instructed to operate as DDS start with carrier, otherwise I/Q mode it is
           iqtest.ModeIQ=MODE_FREQ_A;
        }

        fprintf(stderr,"%s:main() starting operation at frequency(%5.0f) with ModeIQ(%d)\n",PROGRAMID,SetFrequency,iqtest.ModeIQ);

	std::complex<float> CIQBuffer[IQBURST];	
        setWord(&MSW,RUN,true);
	while(getWord(MSW,RUN)==true)
	{
	       int CplxSampleNumber=0;
      static float IQBuffer[IQBURST*2];
 	       int nbread=fread(IQBuffer,sizeof(float),IQBURST*2,iqfile);
	   	   if (nbread>0) {
		      for (int i=0;i<nbread/2;i++)  {
                          if (IQBuffer[i*2]==1111.0 && IQBuffer[i*2+1]==1111.0){  //this is a bogus command to switch into I/Q mode
                             IQBuffer[i*2]=0.0;
                             IQBuffer[i*2+1]=0.0;
                             iqtest.ModeIQ=MODE_IQ;
                             //fprintf(stderr,"%s:main() switch into ModeIQ(%d)\n",PROGRAMID,iqtest.ModeIQ);
                          } 
                          if (fdds==true) {
                             if (IQBuffer[i*2]==2222.0 && IQBuffer[i*2+1]==2222.0){ //this is a bogus command to switch into a Freq-Amplitude mode
                                IQBuffer[i*2]=0.0;
                                IQBuffer[i*2+1]=0.0;
                                iqtest.ModeIQ=MODE_FREQ_A;
                                //fprintf(stderr,"%s:main() switch into ModeIQ(%d)\n",PROGRAMID,iqtest.ModeIQ);
                             }
                          }
                          if (IQBuffer[i*2]==3333.0){  //this is a bogus command to change the carrier level
                             driveDDS=IQBuffer[i*2+1];
                             IQBuffer[i*2]=0.0;
                             IQBuffer[i*2+1]=0.0;
                          } 

                          if (IQBuffer[i*2]==4444.0){  //this is a bogus command to change the frequency
                             SetFrequency=IQBuffer[i*2+1];

                             iqtest.clkgpio::disableclk(4);
                             iqtest.clkgpio::SetAdvancedPllMode(true);
                             iqtest.clkgpio::SetCenterFrequency(SetFrequency,SampleRate);
                             iqtest.clkgpio::SetFrequency(0);
                             iqtest.clkgpio::enableclk(4);

                             IQBuffer[i*2]=0.0;
                             IQBuffer[i*2+1]=0.0;
                          } 

 
		          if (iqtest.ModeIQ==MODE_FREQ_A) {  //if into Frequency-Amplitude mode then only drive a constant carrier
                             IQBuffer[i*2]=10.0;   //should be 10 Hz 
                             IQBuffer[i*2+1]=driveDDS;
                          }
		          CIQBuffer[CplxSampleNumber++]=std::complex<float>(IQBuffer[i*2],IQBuffer[i*2+1]);
                      } //endfor
		   } else {
		     printf("%s:main() End of file\n",PROGRAMID);
		     if (loop_mode_flag) fseek ( iqfile , 0 , SEEK_SET ); else 	setWord(&MSW,RUN,false);
	   	   }
		   iqtest.SetIQSamples(CIQBuffer,CplxSampleNumber,Harmonic);
	}
	iqtest.stop();
}

