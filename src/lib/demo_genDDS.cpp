/*  demo_genDDS is a test DDS generator implemented using the librpitx library
    this program is a demonstration of the genDDS object part of the OrangeThunder platform
    This code is largely based upon rpitxv10
    Copyright (C) 2020 Pedro Colla (LU7DID) lu7did@gmail.com
    Copyright (C) 2015-2018  Evariste COURJAUD F5OEO (evaristec@gmail.com)
    Transmitting on HF band is surely not permitted without license (Hamradio for example).
    Usage of this software is not the responsability of the author.
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
   Thanks to first test of RF with Pifm by Oliver Mattos and Oskar Weigl 	
   INSPIRED BY THE IMPLEMENTATION OF PIFMDMA by Richard Hirst <richardghirst@gmail.com>  December 2012
   Helped by a code fragment by PE1NNZ (http://pe1nnz.nl.eu.org/2013/05/direct-ssb-generation-on-pll.html)
 */

#include <unistd.h>
#include "/home/pi/librpitx/src/librpitx.h"
#include "stdio.h"
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include "/home/pi/OrangeThunder/src/lib/genDDS.h"

//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="demo_genDDS";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

byte    TRACE=0x02;
byte    MSW=0x00;
byte    bSignal=0x00;

genDDS* g=nullptr;
struct  sigaction sigact;

int     a;
int     anyargs = 0;
//int     SampleRate=48000;
int     SampleRate=8000;
float   f=14074000;//1MHZ
float   ppmpll=0.0;
int     SetDma=0;
char    port[80];
float   rit=0.0;
float   offset=0.0;
float*  buffer;

samplerf_t RF[8192];

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
static void terminate(int signum)
{
        fprintf(stderr, "\n%s:sighandler() Signal caught(%d), exiting!\n",PROGRAMID,signum);
        setWord(&MSW,RUN,false);
        if (bSignal>0x00) {
           fprintf(stderr, "\n%s:sighandler() Signal terminate re-entrancy, exiting!\n",PROGRAMID,signum);
           exit(16);
        }
        bSignal++;
}
// ======================================================================================================================
// print_usage
// ======================================================================================================================
void print_usage(void)
{

fprintf(stderr,"%s -f [freq] -p [ppm] -s [samplerate] -r [rit] -o [offset/shift] -t [tracelevel][-h]\n",PROGRAMID);

} /* end function print_usage */

static void fatal(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	terminate(0);
}
// ======================================================================================================================
//                                                 MAIN
// ======================================================================================================================
int main(int argc, char* argv[])
{

  fprintf(stderr,"%s version %s build(%s) %s tracelevel(%d)\n",PROGRAMID,PROG_VERSION,PROG_BUILD,COPYRIGHT,TRACE);

  sigact.sa_handler = terminate;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  for (int i = 0; i < 64; i++) {
      struct sigaction sa;

      std::memset(&sa, 0, sizeof(sa));
      sa.sa_handler = terminate;
      sigaction(i, &sa, NULL);
  }
  while(1)
        {
        int ax = getopt(argc, argv, "p:f:r:o:t:s:h");

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
             (TRACE>=0x01 ? fprintf(stderr,"%s: argument frequency(%f)\n",PROGRAMID,f) : _NOP);
             break;
        case 'r': // RIT
             rit = atof(optarg);
             (TRACE>=0x01 ? fprintf(stderr,"%s: argument RIT(%f)\n",PROGRAMID,rit) : _NOP);
             break;
        case 'o': // Frequency
             offset = atof(optarg);
             (TRACE>=0x01 ? fprintf(stderr,"%s: argument offset(%f)\n",PROGRAMID,offset) : _NOP);
             break;
        case 's': // SampleRate (Only needeed in IQ mode)
             SampleRate = atof(optarg);
             (TRACE>=0x01 ? fprintf(stderr,"%s: argument Sample rate(%d)\n",PROGRAMID,SampleRate) : _NOP);
             break;
        case 't': // SampleRate (Only needeed in IQ mode)
             TRACE = atoi(optarg);
             (TRACE>=0x01 ? fprintf(stderr,"%s: argument tracelevel(%d)\n",PROGRAMID,TRACE) : _NOP);
             break;
        case -1:
             break;
        case 'h':
             if (isprint(optopt) )
             {
                (TRACE>=0x00 ? fprintf(stderr, "%s:arg_parse() unknown option `-%c'.\n",PROGRAMID,optopt) : _NOP);
             }  else {
                (TRACE>=0x00 ?fprintf(stderr, "%s:arg_parse() unknown option character `\\x%x'.\n",PROGRAMID,optopt) : _NOP);
             }
             print_usage();
             exit(1);
             break;
        default:
             print_usage();
             exit(1);
             break;
        }/* end switch ax */
        }/* end while getopt() */


  buffer=(float*) malloc(4096*8);

  g = new genDDS(NULL);
  g->setRIT(rit);
  g->setShift(offset);
  g->setFrequency(f);
  g->setSR(SampleRate);

  fprintf(stderr,"%s:main() DDS Object created\n",PROGRAMID);

  g->start();
  fprintf(stderr,"%s:main() DDS Object created\n",PROGRAMID);

  setWord(&MSW,RUN,true);
  fprintf(stderr,"%s:main() start operations f(%f)\n",PROGRAMID,100+(getWord(g->MSW,PTT)==false ? g->rit : g->shift));

uint32_t W=125000;
  
  while(getWord(MSW,RUN)==true)
    {
       //g->createBuffer(buffer,4096);
       //g->sendBuffer(buffer,4096);
       //usleep(512000);
       int SampleNumber=0;
       for (int i=0;i<1024;i++) {
           RF[i].Frequency=100+(getWord(g->MSW,PTT)==true ? g->rit : g->shift);
           RF[i].WaitForThisSample=W;
           buffer[SampleNumber++]=(float)(RF[i].Frequency);
       }

       g->fmsender->SetFrequencySamples(buffer,SampleNumber);
       //usleep(100);
    }
  g->stop();
  delete(g);

}
