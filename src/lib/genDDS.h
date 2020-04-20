//--------------------------------------------------------------------------------------------------
// genDDS DDS implementationr   (HEADER CLASS)
// wrapper to call ngfmdmasync from librpitx in order to implement a DDS functionality
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef genDDS_h
#define genDDS_h

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
#include "/home/pi/librpitx/src/librpitx.h"
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstring>
#include <signal.h>
#define  IQBURST  4096

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();
typedef struct {
             double Frequency;
           uint32_t WaitForThisSample;
} samplerf_t;



bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);

//---------------------------------------------------------------------------------------------------
// DDS CLASS
//---------------------------------------------------------------------------------------------------
class genDDS
{
  public: 
  
         genDDS(CALLBACK upcall);

// --- public methods

CALLBACK upcall=NULL;
    void start();
    void stop();
     int readpipe(char* buffer,int len);
    void setFrequency(float f);
    void setRIT(float r);
    void setShift(float s);
    void setSR(int sr);
    void setPTT(bool v);
    void sendBuffer(float* RfBuffer,int RfSize);
     int createBuffer(float* RfBuffer,int RfSize);

//static samplerf_t RfBuffer[IQBURST];

// -- public attributes

      byte                TRACE=0x02;

      int                 sr=4000;
      float               ppmpll=0.0;
      int                 SetDma=0;
      float               f;
      byte                MSW = 0;

      float               rit=0.0;
      float               shift=0.0;
      padgpio             pad;

      ngfmdmasync         *fmsender=nullptr;
      float               AmOrFmBuffer[IQBURST];
      int                 FifoSize=IQBURST*4;


//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="genDDS";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

private:


};

#endif
//---------------------------------------------------------------------------------------------------
// genDDS CLASS Implementation
//--------------------------------------------------------------------------------------------------
genDDS::genDDS(CALLBACK u){

// -- VOX callback

   if (u!=NULL) {upcall=u;}

// --- initial definitions

   this->f=14074000;
   this->sr=4000;

   setWord(&MSW,RUN,false);
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::genDDS() Object Initialization...\n",PROGRAMID) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// start() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::start() {


     if (fmsender != nullptr) {
        (this->TRACE>=0x02 ? fprintf(stderr,"%s::start() object already started, avoid...\n",PROGRAMID) : _NOP);
        return;
     }
     fmsender=new ngfmdmasync(f,sr,14,FifoSize);
     //fmsender->clkgpio::setlevel(7);
     pad.setlevel(7);

     (this->TRACE>=0x02 ? fprintf(stderr,"%s::start() object started.\n",PROGRAMID) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// sendBuffer() CLASS Implementation
//---------------------------------------------------------------------------------------------------
int  genDDS::createBuffer(float* RfBuffer,int RfSize) {

     if (RfSize == 0) {
        (this->TRACE>=0x02 ? fprintf(stderr,"%s::createBuffer() empty buffer, ignored\n",PROGRAMID) : _NOP);
        return 0;
     }

     for (int i=0;i<RfSize;i++) {
         RfBuffer[i]=100+(getWord(MSW,PTT)==true ? rit : shift);
     }

     return RfSize;
}
//---------------------------------------------------------------------------------------------------
// sendBuffer() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::sendBuffer(float *RfBuffer,int RfSize) {

     if (RfSize == 0) {
        (this->TRACE>=0x02 ? fprintf(stderr,"%s::sendBuffer() empty buffer, ignored\n",PROGRAMID) : _NOP);
        return;
     }

     int SampleNumber=0;
     for(int i=0;i<RfSize;i++)
     {
        AmOrFmBuffer[SampleNumber++]=(float)(RfBuffer[i]);
     }

     fmsender->SetFrequencySamples(AmOrFmBuffer,SampleNumber);
     return;

}
//---------------------------------------------------------------------------------------------------
// stop() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::stop() {

     delete(fmsender);
     fmsender=nullptr;

     (this->TRACE>=0x02 ? fprintf(stderr,"%s::stop() object stopped\n",PROGRAMID) : _NOP);

}
//---------------------------------------------------------------------------------------------------
// setSoundChannel CLASS Implementation
//--------------------------------------------------------------------------------------------------
int  genDDS::readpipe(char* buffer,int len) {


   return 0;
}
//---------------------------------------------------------------------------------------------------
// setFrequency() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::setFrequency(float freq) {


   if (fmsender==nullptr) {
      f=freq;
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setFrequency() DDS object inactive base frequency set to f=%f\n",PROGRAMID,f) : _NOP);
      return;
   }

   if (freq==f) {
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setFrequency() Informed frequency == previous frequency f=%f\n",PROGRAMID,f) : _NOP);
      return;
   }

   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setFrequency() DDS object active reset needed to change f=%f\n",PROGRAMID,f) : _NOP);

   fmsender->clkgpio::disableclk(GPIO_DDS);
   fmsender->clkgpio::SetAdvancedPllMode(true);
   fmsender->clkgpio::SetCenterFrequency(f,sr);
   fmsender->clkgpio::SetFrequency(0);
   fmsender->clkgpio::enableclk(GPIO_DDS);

   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setFrequency() DDS object active reset completed\n",PROGRAMID) : _NOP);



}
//---------------------------------------------------------------------------------------------------
// setRIT() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::setRIT(float r) {

   if (r < 0 || r > 1000) {
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setRIT() invalid RIT (%f) rejected\n",PROGRAMID,r) : _NOP);
      return;
   }
   rit=r;
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setRIT() RIT (%f) accepted\n",PROGRAMID,rit) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setShift() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::setShift(float s) {
   if (s < 0 || s > 1000) {
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setShift() invalid Shift (%f) rejected\n",PROGRAMID,s) : _NOP);
      return;
   }

   shift=s;
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setRIT() Shift (%f) accepted\n",PROGRAMID,shift) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setSR() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::setSR(int s) {

   if (s < 4000 || s >48000 ) {
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSR() invalid Samplerate (%d) rejected\n",PROGRAMID,s) : _NOP);
      return;
   }

   sr=s;
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSR() SampleRate (%d) accepted\n",PROGRAMID,sr) : _NOP);
}
//---------------------------------------------------------------------------------------------------
// setPTT() CLASS Implementation
//--------------------------------------------------------------------------------------------------
void genDDS::setPTT(bool v) {


 if(v==true) {
   setWord(&MSW,PTT,true);
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setPTT() PTT[%s] PTT On\n",PROGRAMID,BOOL2CHAR(v)) : _NOP);

 } else {
   setWord(&MSW,PTT,false);
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::setPTT() PTT[%s] PTT Off\n",PROGRAMID,BOOL2CHAR(v)) : _NOP);
 }

}
