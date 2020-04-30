//--------------------------------------------------------------------------------------------------
// cabrillo logger (HEADER CLASS)
// general purpose logger using the cabrillo format
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef cabrillo_h
#define cabrillo_h

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

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);


//---------------------------------------------------------------------------------------------------
// gpio CLASS
//---------------------------------------------------------------------------------------------------
class cabrillo

{
  public: 
  
         cabrillo(CALLBACK p);

// --- public methods

CALLBACK changeLog=NULL;
    void start();
    void stop();
    void writeHEADER();
    void writeFOOTER();
    void writeQSO(int freq, char* mode, int year,int mon,int day, int hour, int min, char* hiscall, char* hisrst, char* hisexch, char* mycall, char* myrst, char* myexch);

// -- public attributes

      byte                TRACE=0x02;
      byte                MSW = 0;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="cabrillo";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
const char   *CABRILLO_VERSION="3.0";
const char   *CABRILLO_CREATEDBY="ORANGE THUNDER";

char  callsign[32];
char  category[32];
char  op[32];
char  band[32];
char  mode[16];
char  power[32];
char  station[32];
char  transmitter[32];
char  overlay[32];
char  email[64];
char  grid[8];
char  location[128];
char  name[128];
char  address[128];
char  soapbox[256];
char  dataToAppend[1024];
char  filePath[100];

private:

FILE  *fPtr=NULL;

};

#endif
//---------------------------------------------------------------------------------------------------
// gpio CLASS Implementation
//--------------------------------------------------------------------------------------------------
cabrillo::cabrillo(CALLBACK p){


   if (p!=NULL) {changeLog=p;}

   strcpy(callsign,"LU7DID");
   strcpy(category,"ONE");
   strcpy(mode,"DIGI");
   strcpy(band,"ALL");
   strcpy(op,"Pedro");
   strcpy(power,"QRPp");
   strcpy(station,"FIXED");
   strcpy(transmitter,"ONE");
   strcpy(overlay,"BASIC");
   strcpy(email,"lu7did@gmail.com");
   strcpy(grid,"GF05TE");
   strcpy(location,"Adrogue,BA,Argentina");
   strcpy(name,"Pedro");
   strcpy(address,"Adrogue,BA");
   strcpy(soapbox,"OrangeThunder project");
   strcpy(filePath,"./cabrillo.log");
   setWord(&MSW,RUN,false);
   fPtr=NULL;
}
//---------------------------------------------------------------------------------------------------
// start operations (fork processes) Implementation
//--------------------------------------------------------------------------------------------------
void cabrillo::start() {

    sprintf(filePath,"./%s_cabrillo.log",callsign);
    fPtr = fopen(filePath,"r");
    if (fPtr == NULL) {
       (TRACE>=0x00 ? fprintf(stderr,"%s::start() file %s do not exist, create it\n",PROGRAMID,filePath) : _NOP);
       this->writeHEADER();
       return;
    }
    fclose(fPtr);

/*  Open all file in append mode. */

    setWord(&MSW,RUN,true);
    (this->TRACE >=0x01 ? fprintf(stderr,"%s::start() process started\n",PROGRAMID) : _NOP);

  return;
}
//---------------------------------------------------------------------------------------------------
// write to the log Implementation
//--------------------------------------------------------------------------------------------------
void cabrillo::writeQSO(int freq, char* mode, int year,int mon,int day, int hour, int min, char* hiscall, char* hisrst, char* hisexch, char* mycall, char* myrst, char* myexch) {

    fPtr = fopen(filePath, "a");
    if (fPtr == NULL) {
       (TRACE>=0x00 ? fprintf(stderr,"%s::writeQSO() file %s unable to open\n",PROGRAMID,filePath) : _NOP);
       return;
    }
    (TRACE>=0x02 ? fprintf(stderr,"%s::writeQSO() received year(%d) month(%d) day(%d) hour(%d) minute(%d)\n",PROGRAMID,year,mon,day,hour,min) : _NOP);
    sprintf(dataToAppend,"QSO: %5d %2s %04d-%02d-%02d %02d:%02d %13s %3s %6s %13s %3s %6s\n",freq,mode,year,mon,day,hour,min,hiscall,hisrst,hisexch,mycall,myrst,myexch);
    fputs(dataToAppend,fPtr);

    fclose(fPtr);
}
//---------------------------------------------------------------------------------------------------
// write the initial header at the start of the log
//--------------------------------------------------------------------------------------------------
void cabrillo::writeHEADER() {

    fPtr = fopen(filePath, "w");
    if (fPtr == NULL) {
       (this->TRACE >=0x01 ? fprintf(stderr,"%s::writeHEADER() error file not open\n",PROGRAMID) : _NOP);
       return;
    }
    sprintf(dataToAppend,"START-OF-LOG:%s\n",CABRILLO_VERSION);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"CALLSIGN:%s\n",callsign);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"GRID-LOCATOR:%s\n",grid);
    fputs(dataToAppend, fPtr);
// ---
    sprintf(dataToAppend,"CATEGORY-BAND:%s\n",band);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"CATEGORY-POWER:%s\n",power);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"CATEGORY-TRANSMITTER:%s\n",transmitter);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"CATEGORY-OVERLAY:%s\n",overlay);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"CREATED-BY:%s\n",(char*)"OrangeThunder V1.0");
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"LOCATION:%s\n",location);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"NAME:%s\n",name);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"ADDRESS:%s\n",address);
    fputs(dataToAppend, fPtr);

    sprintf(dataToAppend,"SOAPBOX:%s\n",grid);
    fputs(dataToAppend, fPtr);


    fclose(fPtr);
}
void cabrillo::writeFOOTER() {


} 
//---------------------------------------------------------------------------------------------------
// stop CLASS Implementation
//--------------------------------------------------------------------------------------------------
void cabrillo::stop() {

// --- Normal termination kills the child first and wait for its termination

  if (getWord(MSW,RUN)==false) {
     return;
  }

  if (fPtr==NULL) {
     return;
  }

  fclose(fPtr);
  setWord(&MSW,RUN,false);
  (this->TRACE >=0x01 ? fprintf(stderr,"%s::stop() process terminated\n",PROGRAMID) : _NOP);

}
