/*
 * demo_decodeFT8
 *-----------------------------------------------------------------------------
 * Quick refactoring to integrate rtl_fm and decodeFT8 into a single entity
 * as part of a transceiver on the OrangeThunder platform
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

#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 
#include <sys/select.h>
	#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
//#include <ncurses.h>

#include<sys/wait.h>
#include<sys/prctl.h>

#include <iostream>
#include <fstream>
using namespace std;

#include "/home/pi/OrangeThunder/src/lib/decodeFT8.h"
#include "/home/pi/OrangeThunder/src/lib/cabrillo.h"

// --- IPC structures

struct msg {
       char timestamp[16];    // time stamp
       byte slot;           //
       bool CQ;
        int snr;             // Signal SNR
      float DT;             // Time Shift
        int offset;         // Frequency offset
       char t1[16];         // first token
       char t2[16];         // second token
       char t3[16];         // third token
};

struct header {
      char  timestamp[16];
      byte  slot;
      bool  newheader;
      bool  active;
};

struct qso {
      byte  FSM;
      int   nmsg;
      char  hiscall[16];
      char  hisgrid[16];
      char  hissnr[16];
      int   hisslot;
      int   offset;
      char  mycall[16];
      char  mygrid[16];
      char  mysnr[16];
      int   myslot;
      int   cnt;
      char  lastsent[256];
};
struct Queue {
      int   slot;
      char  message[1024];
};

typedef void (*QSOCALL)(header* h,msg* m,qso* q);
QSOCALL    FSMhandler[50];

decodeFT8* r=nullptr;
struct     sigaction sigact;
char       *buffer;

byte       HAL=0x00;
byte       TRACE=0x00;
byte       MSW=0x00;

char       mycall[16];
char       mygrid[16];
char       mysig[16];
int        anyargs;
char       filePath[128];

char       inChar[32];
float      freq=14074000;    // operating frequency
int        sr=12000;

msg        ft8_msg[50];
header     ft8_header[1];
qso        ft8_qso[1];
int        nmsg=0;
int        nCQ=0;

Queue      queueFT8[1024];
int        nextPush=0;
int        nextPop=0;

boolean    fthread=false;
pthread_t  t=(pthread_t)-1;
pthread_t  pift8=(pthread_t)-1;
char       pcmd[1024];
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="demo_decodeFT8";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";
const char   *FT8="F8";
const char   *RST="599";

cabrillo*  l=nullptr;;
// *----------------------------------------------------------------*
// *                  GPIO support processing                       *
// *----------------------------------------------------------------*
// --- gpio object
//gpioWrapper* g=nullptr;
//char   *gpio_buffer;
//void gpiochangePin();

// *----------------------------------------------------------------*
// *           Read commans from Std input while running            *
// *----------------------------------------------------------------*
int       fd;
char      len;
char      buff[128];


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
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------

char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------

char *trim(char *s)
{
    return rtrim(ltrim(s)); 
}
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------

void clearQueue() {

     nextPop=0;
     nextPush=0;
    (TRACE>=0x03 ? fprintf(stderr,"%s::pushQueue() clear stack\n",PROGRAMID) : _NOP);

     return;
}
//--------------------------------------------------------------------------------------------------
// setSSW Sets a given bit of the system status Word (SSW)
//--------------------------------------------------------------------------------------------------

void pushQueue(int slot,char* cmd) {

     queueFT8[nextPush].slot=slot;
     strcpy(queueFT8[nextPush].message,cmd);
    (TRACE>=0x02 ? fprintf(stderr,"%s::pushQueue() stored stack(%d) message(%s)\n",PROGRAMID,nextPush,queueFT8[nextPush].message) : _NOP);

     nextPush++;
     if (nextPush>=1024) {
        nextPush=0;
     }
     return;

}
//--------------------------------------------------------------------------------------------------
// popQueue recover an element from the message queue
//--------------------------------------------------------------------------------------------------
int popQueue() {
int i=nextPop;
    if(i==nextPush) {
      return -1;
    }
    nextPop++;
    if (nextPop>=1024) {
       nextPop=0;
    }
    (TRACE>=0x02 ? fprintf(stderr,"%s::popQueue() retrieved stack(%d) message(%s)\n",PROGRAMID,i,queueFT8[i].message) : _NOP);
    return i;
}
//--------------------------------------------------------------------------------------------------
// isQueue return whether there is a pending element or not
//--------------------------------------------------------------------------------------------------
bool isQueue() {
    if (nextPop != nextPush) {
       return true;
    }
    return false;
}
//--------------------------------------------------------------------------------------------------
// peekQueue peek for next element
//--------------------------------------------------------------------------------------------------
int peekQueue() {
    return nextPop;
}
//--------------------------------------------------------------------------------------------------
// writeQSO in log if available
//--------------------------------------------------------------------------------------------------
void writeLog(header* h,msg* m, qso* q) {

     if (l!=nullptr) {
        time_t theTime = time(NULL);
        struct tm *aTime = localtime(&theTime);
           int year=aTime->tm_year+1900;
           int month=aTime->tm_mon+1;
           int day=aTime->tm_mday;
           int hour=aTime->tm_hour;
           int min=aTime->tm_min;
           l->writeQSO(freq/1000,(char*)FT8,year,month,day,hour,min,q->hiscall,(char*)RST,q->hissnr,q->mycall,(char*)RST,q->hissnr);
      }
      return;
}
//---------------------------------------------------------------------------------------------------
// startProcess
// thread to send information using FT8 frames over the air
//---------------------------------------------------------------------------------------------------
void* startProcess (void * null) {

    (TRACE>=0x02 ? fprintf(stderr,"%s::startProcess() starting thread\n",PROGRAMID) : _NOP);
    char cmd[2048];
    char buf[4096];

    sprintf(cmd,"python /home/pi/OrangeThunder/bash/turnon.py 2>/dev/null && %s 2>/dev/null > /dev/null  && python /home/pi/OrangeThunder/bash/turnoff.py 2>/dev/null",pcmd);
    (TRACE>=0x02 ? fprintf(stderr,"%s::startProcess() cmd[%s]\n",PROGRAMID,cmd) : _NOP);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        (TRACE>=0x00 ? fprintf(stderr,"%s::startProcess() error opening pipe\n",PROGRAMID) : _NOP);
        fthread=false;
        pthread_exit(NULL);
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        // fprintf(stderr,"o-> %s", buf);
    }

    if(pclose(fp))  {
        (TRACE>=0x00 ? fprintf(stderr,"%s::startProcess() command not found of exited with error status\n",PROGRAMID) : _NOP);
        fthread=false;
        pthread_exit(NULL);
    }

    (TRACE>=0x02 ? fprintf(stderr,"%s::startProcess() thread normally terminated\n",PROGRAMID) : _NOP);
    fthread=false;
    pthread_exit(NULL);
}

// --------------------------------------------------------------------------------------------------
// sendFT8
// send a frame pending on the queue
// --------------------------------------------------------------------------------------------------
void sendFT8(header* h) {

    if (fthread == true) {
       (TRACE >= 0x02 ? fprintf(stderr,"%s::sendFT8() thread already running, skip!\n",PROGRAMID) : _NOP);
       return;
    }


    if (isQueue() == false) {   // and it is for this particular slot then send, otherwise leave
       (TRACE >= 0x02 ? fprintf(stderr,"%s::sendFT8() slot(%d) queue empty, skip!\n",PROGRAMID,h->slot) : _NOP);
       return;
    }

    if (queueFT8[peekQueue()].slot != h->slot) {
       (TRACE >= 0x02 ? fprintf(stderr,"%s::sendFT8() slot(%d) waiting till slot(%d), skip!\n",PROGRAMID,h->slot,queueFT8[peekQueue()].slot) : _NOP);
       return;
    }

    int k=popQueue();    // Check if there is any pending message, if so continue
    if (k==-1) {
       return;
    }

    (TRACE>=0x02 ? fprintf(stderr,"%s::sendFT8() slot(%d) sending stack(%d) message(%s)\n",PROGRAMID,h->slot,k,queueFT8[k].message) : _NOP);
    
    fthread=true;
    sprintf(pcmd,"python /home/pi/OrangeThunder/bash/turnon.py && sudo %s && python /home/pi/OrangeThunder/bash/turnoff.py",queueFT8[k].message);
int rct=pthread_create(&pift8,NULL, &startProcess, NULL);
    if (rct != 0) {
       (TRACE>=0x00 ? fprintf(stderr,"%s:main() thread can not be created at this point rc(%d)\n",PROGRAMID,rct) : _NOP);
       exit(16);
    }
// --- process being launch 


   (TRACE >= 0x03 ? fprintf(stderr,"%s::sendFT8() thread launch completed[%d]\n",PROGRAMID,rct) : _NOP);
}
// --------------------------------------------------------------------------------------------------
void retryFT8(qso* q) {

    if (isQueue()==true) {
       return;
    }

    q->cnt--;
    if (q->cnt==0) {
       q->FSM=0x00;
       clearQueue();
       (TRACE>=0x02 ? fprintf(stderr,"%s:retryFT8() FSM(%d) **retry exceeded***\n",PROGRAMID,q->FSM) : _NOP);
    }
    (TRACE>=0x02 ? fprintf(stderr,"%s:retryFT8() FSM(%d) retrying message slot(%d) [%s]\n",PROGRAMID,q->FSM,q->myslot,q->lastsent) : _NOP);
    pushQueue(q->myslot,q->lastsent);

    return;
}

// --------------------------------------------------------------------------------------------------

void handleNOP(header* h,msg* m,qso* q) {
    (TRACE>=0x02 ? fprintf(stderr,"%s:handleNOP() do nothing hook\n",PROGRAMID) : _NOP);
    return;
}
// --------------------------------------------------------------------------------------------------
void handleStartQSO(header* h,msg* m,qso* q) {

   (TRACE>=0x02 ? fprintf(stderr,"%s:handleStartQSO() handle start of QSO\n",PROGRAMID) : _NOP);
   if (strcmp(m[q->nmsg].t1,"CQ")==0) {   // is a CQ
      if (strcmp(m[q->nmsg].t2,mycall) == 0) { // is CQ mycall * ignore
         (TRACE>=0x02 ? fprintf(stderr,"%s:handleStartQSO() CQ performed by me, ignore\n",PROGRAMID) : _NOP);
         return;
      }

      q->FSM=0x01;
      strcpy(q->hiscall,m[q->nmsg].t2);
      strcpy(q->hisgrid,m[q->nmsg].t3);
      //sprintf(mysig,"R-%s",mysig);,
      strcpy(q->hissnr,mysig);
      strcpy(q->mycall,mycall);
      strcpy(q->mygrid,mygrid);
      strcpy(q->mysnr,(char*)"");
      q->cnt=4;
      q->hisslot=m[q->nmsg].slot;
      q->myslot=(q->hisslot + 1)%2;
      q->offset=m[q->nmsg].offset;
      (TRACE>=0x02 ?  fprintf(stderr,"%s:handleStartQSO() QSO created FSM(%d) [%s %s %s] (slot %d)->[%s %s %s] (slot %d) cnt(%d)\n",PROGRAMID,q->FSM,q->hiscall,q->hisgrid,q->hissnr,q->hisslot,q->mycall,q->mygrid,q->mysnr,q->myslot,q->cnt) : _NOP);

//--- push a response in the message queue to start the QSO

      clearQueue();
      sprintf(q->lastsent,"pift8 -m \"%s %s %s\" -f %f -s %d -o %d",q->hiscall,q->mycall,q->mygrid,freq,q->myslot,q->offset);
      (TRACE>=0x02 ? fprintf(stderr,"%s:handleStartQSO() FSM(%d) reply(%s)\n",PROGRAMID,q->FSM,q->lastsent) : _NOP);
      pushQueue(q->myslot,q->lastsent);
      return;
   }

   return;

}
// --------------------------------------------------------------------------------------------------
void handleReply(header* h,msg* m,qso* q) {

    (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() FSM(%d) QSO (%s(%d):%s(%d))\n",PROGRAMID,q->FSM,q->hiscall,q->hisslot,q->mycall,q->myslot) : _NOP);

//--- scan all available messages

    for (int i=0;i<nmsg;i++) {
        (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() FSM(%d) message (%d)<%s %s %s>\n",PROGRAMID,q->FSM,i,m[i].t1,m[i].t2,m[i].t3) : _NOP);

        if (strcmp(m[i].t1,"CQ")==0) {                //it is a CQ call
           if (strcmp(m[i].t2,q->hiscall)==0) {       //CQ hiscall *
              (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() CQ hiscall *, retry(%s)\n",PROGRAMID,q->lastsent) : _NOP);
              retryFT8(q);                 //No pending message, so the previous one has been sent but not received, retry
              return;                                 //process no further message
           }

           if (strcmp(m[i].t2,q->mycall)==0) {        // CQ mycall this is weird, ignore
              continue;
           }

           continue;
        }

        if (strcmp(m[i].t1,mycall)==0) {
           if (strcmp(m[i].t2,q->hiscall)==0) {       // Mycall HisCall * sounds like an answer
              strcpy(q->mysnr,m[i].t3);
              clearQueue();
              if (isQueue()==false) {
                 sprintf(q->lastsent,"pift8 -m \"%s %s %s\" -f %f -s %d -o %d",q->hiscall,q->mycall,q->hissnr,freq,q->myslot,q->offset);
                 pushQueue(q->myslot,q->lastsent);
                 (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() mycall hiscall SIG , reply(%s)\n",PROGRAMID,q->lastsent) : _NOP);
                 q->FSM=0x02;                         // advance to next stage in the machine
              }
              return;
           }
           continue;
        }


        if (strcmp(m[i].t1,mycall)!=0 && strcmp(m[i].t1,q->hiscall)!=0) {  //not my or his call, somebody elses call
           if (strcmp(m[i].t2,q->hiscall)==0) {                                       //from his call, so clearly doing QSO with somebody else
              (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() !mycall hiscall *, abandon\n",PROGRAMID) : _NOP);
              q->FSM=0x00;
              clearQueue();
              return;
           }
           continue;
        }

    }
//--- end of loop so all messages has been scanned and no appropriate action found before, so is there is no pending action resend message

    (TRACE>=0x02 ? fprintf(stderr,"%s:handleReply() FSM(%d) processing retry(%s)\n",PROGRAMID,q->FSM,q->lastsent) : _NOP);
    retryFT8(q);

}
// --------------------------------------------------------------------------------------------------
void handleRRR(header* h,msg* m,qso* q) {


    (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() FSM(%d) QSO (%s(%d):%s(%d))\n",PROGRAMID,q->FSM,q->hiscall,q->hisslot,q->mycall,q->myslot) : _NOP);

    for (int i=0;i<nmsg;i++) {
       (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() FSM(%d) line---------> (%s %s %s)\n",PROGRAMID,q->FSM,m[i].t1,m[i].t2,m[i].t3) : _NOP);
 
       if (strcmp(m[i].t1,q->mycall)==0) {       //mycall * *
          if (strcmp(m[i].t2,q->hiscall)==0) {   //mycall hiscall *
             if (strcmp(m[i].t3,"RRR")==0 || strcmp(m[i].t3,"73")==0 || strcmp(m[i].t3,"R73")==0 || strcmp(m[i].t3,"RR73") ==0) {    //mycall hiscall {R73|73|RRR|RR73}
                 clearQueue();
                 if (isQueue()==false) {
                    sprintf(q->lastsent,"pift8 -m \"%s %s %s\" -f %f -s %d -o %d",q->hiscall,q->mycall,"RR73",freq,q->myslot,q->offset);
                    pushQueue(q->myslot,q->lastsent);
                    (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() mycall hiscall RRR, reply (%s) and finalize\n",PROGRAMID,q->lastsent) : _NOP);
                    writeLog(h,m,q);
                    q->FSM=0x00;
                    return;
                 }
                 (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() mycall hiscall *, retry (%s)\n",PROGRAMID,q->lastsent) : _NOP);
               
                 retryFT8(q);
                 return;  
             }
             return;
          }
          continue;
       }


       if (strcmp(m[i].t1,q->mycall)!=0 && strcmp(m[i].t1,q->hiscall)!=0) {    //not his call nor mine
          if (strcmp(m[i].t2,q->hiscall)==0) {         // hiscall * answering to somebody else
             (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() * hiscall *, abandon\n",PROGRAMID) : _NOP);
             q->FSM=0x00;
             clearQueue();
             return;
          }
          continue;
       }

       if (strcmp(m[i].t1,"CQ")==0) {    // CQ *
          if (strcmp(m[i].t2,q->hiscall)==0) {         // CQ hiscall * ignoring my QSO or operating as if terminated, abandon
             (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() CQ hiscall *, abandon\n",PROGRAMID) : _NOP);
             q->FSM=0x00;
             clearQueue();
             return;
          }
          continue;
       }

    }
    (TRACE>=0x02 ? fprintf(stderr,"%s:handleRRR() retry(%s)\n",PROGRAMID,q->lastsent) : _NOP);
    retryFT8(q);


}
// --------------------------------------------------------------------------------------------------

void handleCQ(header* h,msg* m,qso* q) {

      (TRACE>=0x02 ? fprintf(stderr,"%s:handleCQ() handle CQ\n",PROGRAMID) : _NOP);

      strcpy(q->hiscall,"");
      strcpy(q->hisgrid,"");
      //sprintf(mysig,"R-%s",mysig);
      strcpy(q->hissnr,mysig);
      strcpy(q->mycall,mycall);
      strcpy(q->mygrid,mygrid);
      strcpy(q->mysnr,(char*)"");
      q->cnt=4;
      q->offset=1240;
      q->myslot=h->slot;
      q->hisslot=(q->hisslot + 1)%2;
      strcpy(q->lastsent,"");
      clearQueue();

      sprintf(q->lastsent,"pift8 -m \"CQ %s %s\" -f %f -s %d ",q->mycall,q->mygrid,freq,q->myslot);    //CQ mycall mygrid
      (TRACE>=0x02 ? fprintf(stderr,"%s:handleCQ() FSM(%d) reply(%s)\n",PROGRAMID,q->FSM,q->lastsent) : _NOP);
      pushQueue(q->myslot,q->lastsent);
      q->FSM=0x04;

      return;

}
// --------------------------------------------------------------------------------------------------

void handleExch(header* h,msg* m,qso* q) {
    (TRACE>=0x02 ? fprintf(stderr,"%s:handleExch() FSM(%d) QSO (%s(%d):%s(%d))\n",PROGRAMID,q->FSM,q->hiscall,q->hisslot,q->mycall,q->myslot) : _NOP);

    for (int i=0;i<nmsg;i++) {
        if (strcmp(m[i].t1,q->mycall)==0) {     //mycall hiscall hisgrid
           strcpy(q->hiscall,m[i].t2);
           strcpy(q->hisgrid,m[i].t3);
           clearQueue();
           sprintf(q->lastsent,"pift8 -m \"%s %s %s\" -f %f -s %d ",q->hiscall,q->mycall,q->hissnr,freq,q->myslot);
           (TRACE>=0x02 ? fprintf(stderr,"%s:handleExch() FSM(%d) reply(%s)\n",PROGRAMID,q->FSM,q->lastsent) : _NOP);
           pushQueue(q->myslot,q->lastsent);
           q->FSM=0x05;
           return;
        }
    }

    retryFT8(q);
    return;
}
// --------------------------------------------------------------------------------------------------
void handle73(header* h,msg* m,qso* q) {

    (TRACE>=0x02 ? fprintf(stderr,"%s:handle73() FSM(%d) QSO (%s(%d):%s(%d))\n",PROGRAMID,q->FSM,q->hiscall,q->hisslot,q->mycall,q->myslot) : _NOP);

    for (int i=0;i<nmsg;i++) {
        if (strcmp(m[i].t1,q->mycall)==0) {
           if (strcmp(m[i].t2,q->hiscall) == 0) {    //mycall hiscall mysnr
              strcpy(q->mysnr,m[i].t3);
              clearQueue();
              sprintf(q->lastsent,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,"RR73",freq,q->myslot);
              (TRACE>=0x02 ? fprintf(stderr,"%s:handle73() FSM(%d) reply(%s)\n",PROGRAMID,q->FSM,q->lastsent) : _NOP);
              pushQueue(q->myslot,q->lastsent);
              q->FSM=0x06;

              return;
           }
        }
    }

    retryFT8(q);
    return;

}
// --------------------------------------------------------------------------------------------------
void handleTU(header* h,msg* m,qso* q) {

    (TRACE>=0x02 ? fprintf(stderr,"%s:handleEXCH() FSM(%d) QSO (%s(%d):%s(%d))\n",PROGRAMID,q->FSM,q->hiscall,q->hisslot,q->mycall,q->myslot) : _NOP);

    for (int i=0;i<nmsg;i++) {
        if (strcmp(m[i].t1,q->mycall)==0) {
           if (strcmp(m[i].t2,q->hiscall) == 0) {    //mycall hiscall {R73 | RRR | RR73}
              if (strcmp(m[i].t3,"RRR")==0 || strcmp(m[i].t3,"73")==0 || strcmp(m[i].t3,"R73")==0 || strcmp(m[i].t3,"RR73") ==0) {    //mycall hiscall {R73|73|RRR|RR73}
                 q->FSM=0x00;
                 writeLog(h,m,q);
                 clearQueue();
                 return;
              }
           }
        }
    }

    retryFT8(q);
    return;

}

// --------------------------------------------------------------------------------------------------

void handleCancelQSO(header* h,msg* m,qso* q) {
    (TRACE>=0x01 ? fprintf(stderr,"%s:handleHAL() handle cancel QSO behaviour\n",PROGRAMID) : _NOP);

    q->FSM=0x00;
    clearQueue();
    strcpy(q->hiscall,"");
    strcpy(q->hisgrid,"");
    strcpy(q->hissnr,"");
    strcpy(q->mycall,mycall);
    strcpy(q->mygrid,mygrid);
    strcpy(q->mysnr,(char*)"");
    q->cnt=4;
    q->offset=1240;
    q->myslot=0;
    q->hisslot=1;
    strcpy(q->lastsent,"");

    return;

}

// --------------------------------------------------------------------------------------------------

void handleHAL(header* h,msg* m,qso* q) {
    (TRACE>=0x00 ? fprintf(stderr,"%s:handleHAL() handle HAL behaviour\n",PROGRAMID) : _NOP);
    return;

}


// --------------------------------------------------------------------------------------------------
// qsoMachine
// --------------------------------------------------------------------------------------------------
int qsoMachine(header* h,msg* m,qso* q) {

    if (q->FSM >= 0 && q->FSM <= 50) {
       (TRACE>=0x03 ? fprintf(stderr,"%s:qsoMachine() messages(%d) to process FSM(%d)\n",PROGRAMID,nmsg,q->FSM) : _NOP);
       for (int j=0;j<nmsg;j++) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:qsoMachine()      [%s %s %s]\n",PROGRAMID,m[j].t1,m[j].t2,m[j].t3) : _NOP);
       }
       
       if (FSMhandler[q->FSM] != nullptr) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:qsoMachine() service handler found for FSM state(%d) calling\n",PROGRAMID,q->FSM) : _NOP);
          FSMhandler[q->FSM](h,m,q);
          return 0;
       } else {
          (TRACE>=0x00 ? fprintf(stderr,"%s:qsoMachine() handler for FSM state(%d) not defined\n",PROGRAMID,q->FSM) : _NOP);
         return -1;
       }
       (TRACE>=0x00 ? fprintf(stderr,"%s:qsoMachine() invalid FSM state(%d) ignored\n",PROGRAMID,q->FSM) : _NOP);
       return -1;
    }
    return  0;
}

//--------------------------------------------------------------------------------------------------
// Process QSO
//--------------------------------------------------------------------------------------------------
int  selectQSO(char* c) {

int r=atoi(c);

    (TRACE>=0x03 ? fprintf(stderr,"%s:selectQSO() QSO[%d]\n",PROGRAMID,r) : _NOP);

    if (nCQ==0) {
       (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() QSO[%d] not available\n",PROGRAMID,r) : _NOP);
       return -1;
    }

    if (r>=0 && r<nCQ) {
       (TRACE>=0x02 ? fprintf(stderr,"%s:selectQSO() option selected[%d]\n",PROGRAMID,r) : _NOP);
       return r;
    } else {
       (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() QSO[%d] out of range\n",PROGRAMID,r) : _NOP);
       return -1;
    }

 
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


// ======================================================================================================================
// sighandler
// ======================================================================================================================
static void sighandler(int signum)
{
        if (getWord(MSW,RETRY)==true) {
   	   fprintf(stderr, "\nSignal caught(%d), re-entrancy detected forcing termination!\n",signum);
           r->stop();
           delete(r);
           exit(16);
        }
	fprintf(stderr, "\nSignal caught(%d), exiting!\n",signum);
        if(t!=(pthread_t)-1) {
          pthread_cancel(t);
        }
	setWord(&MSW,RUN,false);
        setWord(&MSW,RETRY,true);     
}

int findQSO(int cq,msg *m) {
    int j=0;
    for (int i=0;i<nmsg;i++) {
        if (m[i].CQ==true) {
           if (j==cq) {
              return i;
           }
           j++;
        }
    }
    return -1;
}
// *----------------------------------------------------------------------------------------------------------------
// * clear all messages from the previous cycle
// * messages must be available during most of the next cycle till a new decode message is received
// *----------------------------------------------------------------------------------------------------------------
void clearmsg(msg *m) {

  for (int i=0;i<50;i++) {
    sprintf(m[i].timestamp,"%s",(char*)"");
    m[i].slot=0;
    m[i].CQ=false;
    m[i].snr=0;
    m[i].DT=0.0;
    m[i].offset=0;
    sprintf(m[i].t1,"%s","");
    sprintf(m[i].t2,"%s","");
    sprintf(m[i].t3,"%s","");
  }
  nmsg=0;

}
// *----------------------------------------------------------------------------------------------------------------
// * parse slot header information
// *----------------------------------------------------------------------------------------------------------------
void parseHeader(char* st,header* h,qso* q) {

char * p;
char   aux[32];

       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() received header(%s)\n",PROGRAMID,st) : _NOP);
       h->active=true;

       p=strtok(st,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() malformed header, ignored\n",PROGRAMID) : _NOP);
          return;
       }

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }
          
       sprintf(aux,"%s",p);  //timeMsg
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed timeslot=%s\n",PROGRAMID,aux) : _NOP);
       strcpy(h->timestamp,aux);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed seq=%s\n",PROGRAMID,aux) : _NOP);
       int seq=atoi(aux);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }

       sprintf(aux,"%s",p);  //timeMsg
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed slot=%s\n",PROGRAMID,aux) : _NOP);
       (strcmp(aux,"e")==0 ? h->slot = 0 : h->slot = 1);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }

       sprintf(aux,"%s",p);  //timeMsg
       int status=0;
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed status=%s\n",PROGRAMID,aux) : _NOP);
       (strcmp(aux,"OK")==0 ? status = 0 : status = 1);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       int brk=0;
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed cpl=%s\n",PROGRAMID,aux) : _NOP);
       (strcmp(aux,"cpl")==0 ? brk = 0 : brk = 1);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed candidates=%s\n",PROGRAMID,aux) : _NOP);
       int candidates=atoi(aux);

       p=strtok(NULL,",");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseHeader() parsed decoded=%s\n",PROGRAMID,aux) : _NOP);
       int decoded=atoi(aux);

       if (q->FSM==0x00) {
          fprintf(stderr,"\033[0;31m----[%s] (seq=%d slot=%d) (%d/%d) (%03d- %02d) active(%s)\033[0m\n",(char*)h->timestamp,seq,h->slot,status,brk,candidates,decoded,BOOL2CHAR(h->active));
       } else {
          fprintf(stderr,"\033[1;31m----[%s] (seq=%d slot=%d) (%d/%d) (%03d- %02d) active(%s)\033[0m\n",(char*)h->timestamp,seq,h->slot,status,brk,candidates,decoded,BOOL2CHAR(h->active));
       }
       sendFT8(h);
}
// *----------------------------------------------------------------------------------------------------------------
// * parse message information
// *----------------------------------------------------------------------------------------------------------------
int parseMessage(char* st,header* h,int j,msg* m,qso* q) {

char * p;
char * x;
char    aux[32];
msg    f;


       (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage msg(%d) so far buffer(%s)\n",PROGRAMID,j,st) : _NOP);

       p=strtok(st,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage wrong format\n",PROGRAMID) : _NOP );
          return j;
       }

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage quite wrong format\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.timestamp,"%s",p);  //timeMsg

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage incomplete message\n",PROGRAMID) : _NOP );
          return j;
       }

       sprintf(aux,"%s",p);
       f.snr=atoi(trim(aux));
       if (f.snr==-1) {
          (TRACE>=0x02 ? fprintf(stderr,"%s:processMessage last message FSM(%d)\n",PROGRAMID,q->FSM) : _NOP );
          return -1;
       }

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage corrupted message\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(aux,"%s",p);
       
       f.DT=atof(trim(aux));

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage partial message not processed\n",PROGRAMID) : _NOP );
          return j;
       }

       sprintf(aux,"%s",p);
       f.offset=atoi(trim(aux));


       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage parsing error (t1)\n",PROGRAMID) : _NOP );
          return j;
       }
char   mssg[128];
       sprintf(mssg,"%s",p);

       x=strtok(mssg," ");
       if (x==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage parsing error (t2)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t1,"%s",x);

       x=strtok(NULL," ");
       if (x==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage parsing error (t3)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t2,"%s",x);

       x=strtok(NULL," ");
       if (x==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage parsing error (t3)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t3,"%s",x);
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseMessage()  %s %d %f %d %s %s %s\n",PROGRAMID,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3) : _NOP);

       if (strcmp(mycall,f.t2)==0) { // message sent by me just ignore it
          (TRACE>=0x03 ? fprintf(stderr,"%s:parseMessage()  message sent by me ignored(%s %d %f %d %s %s %s)\n",PROGRAMID,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3) : _NOP);
          return j;
       }


       strcpy(m[j].timestamp,f.timestamp);
       m[j].snr=f.snr;
       m[j].DT=f.DT;
       m[j].offset=f.offset;
       m[j].slot=h->slot;
       strcpy(m[j].t1,f.t1);
       strcpy(m[j].t2,f.t2);
       strcpy(m[j].t3,f.t3);
       m[j].CQ=false;

// --- this is CQ traffic 

       if (strcmp("CQ",f.t1)==0) {
          m[j].CQ=true;
          if (q->FSM>0 && strcmp(q->hiscall,f.t2)==0) {
             fprintf(stderr,"\033[1;33m[%02d] %s %4d %4.1f %04d %s %s %s\033[0m\n",nCQ,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          } else {
             fprintf(stderr,"\033[0;32m[%02d] %s %4d %4.1f %04d %s %s %s\033[0m\n",nCQ,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          }
          nCQ++;
          j++;
          return j;
       }

// --- this is traffic sent to me 

       if (strcmp(mycall,f.t1)==0) { 
          j++;
          if (isQueue()==true) {
             fprintf(stderr,"\033[1;36m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          } else {
             fprintf(stderr,"\033[0;36m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          }
          return j;
       }

// -- this is general traffic

       fprintf(stderr,"\033[0;32m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
       j++;
       return j;
}

// ======================================================================================================================
// processFrame
// manipulate received frames to operate FT8
// =======================================================================================================================
void processFrame(char* s,header* h,msg* m,qso* q) {

char * p;
char d[256];

      (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() processing frame(%s)\n",PROGRAMID,s) : _NOP ); 
   
      if (s==NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() empty frame received, ignored\n",PROGRAMID) : _NOP ); 
         return;
      }


      if (strstr(s,"HDR,") != NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() hdr message(%s)\n",PROGRAMID,s) : _NOP);
         parseHeader(s,h,q);
         h->active=true;
         h->newheader=true;
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() hdr initialized new header(%s)\n",PROGRAMID,BOOL2CHAR(h->newheader)) : _NOP);
         return;
      }

      if (strstr(s,"MSG,") != NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() mssg message(%s) active(%s) new Header(%s)\n",PROGRAMID,s,BOOL2CHAR(h->active),BOOL2CHAR(h->newheader)) : _NOP);
         if (h->newheader==true) {
            h->newheader=false;
            h->active=false;
            clearmsg(m);
            nmsg=0;
            nCQ=0;
            (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() first message, init messages\n",PROGRAMID) : _NOP);
         }

         int n=parseMessage(s,h,nmsg,m,q);
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() message received, pending(%d) there are (%d) in queue\n",PROGRAMID,nmsg,n) : _NOP);
         if (n==-1) {
            (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() last message received, there are (%d) in queue\n",PROGRAMID,nmsg) : _NOP);
            if (q->FSM >= 0x01) {       // there is a QSO in progress
               (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() QSO in progress FSM(%d) process\n",PROGRAMID,q->FSM) : _NOP);
               qsoMachine(h,m,q);
            }
         } else {
           nmsg=n;
           (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() message received, pending(%d) now (%d) in queue\n",PROGRAMID,nmsg,n) : _NOP);
         }
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() number of messages(%d)\n",PROGRAMID,nmsg) : _NOP);

         return; 
      } 

// -- Print as general traffic (not QSO related)

      fprintf(stderr,"\033[0;32m%s\033[0m\n",s);
      return;
 
}
//---------------------------------------------------------------------------------
// Print usage
//---------------------------------------------------------------------------------
void print_usage(void)
{
fprintf(stderr,"%s %s [%s]\n\
Usage: [-f frequency] [-g grid] [-v trace] [-s samplerate] [-l log path] [-x] [-c callsign] [-r report] \n\
-g            grid locator (default GF05)\n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-v            tracelevel (0 to 3)\n\
-s            sample rate (default 12000 beware of modification!)\n\
-l            cabrillo log path (default ./)\n\
-x            enable experimental HAL mode (default disabled)\n\
-c            callsign (default LU7DID)\n\
-r            default signal report (default 10 is R-10)\n\
-?            help (this help).\n\
\n",PROGRAMID,PROG_VERSION,PROG_BUILD);

} /* end function print_usage */

// ======================================================================================================================
// MAIN
// Create IPC pipes, launch child process and keep communicating with it
// ======================================================================================================================
int main(int argc, char** argv)
{
  sigact.sa_handler = sighandler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;

  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  signal(SIGPIPE, SIG_IGN);

  for (int i = 0; i < 64; i++) {
      if (i != SIGALRM && i != 17 && i != 28) {
         signal(i,sighandler);
      }
  }

  for (int i = 0; i < 50; i++) {
      FSMhandler[i]=nullptr;
  }

// -- define processor handles


  FSMhandler[0] = handleStartQSO;
  FSMhandler[1] = handleReply;
  FSMhandler[2] = handleRRR;;
  FSMhandler[3] = handleCQ;
  FSMhandler[4] = handleExch;
  FSMhandler[5] = handle73;
  FSMhandler[6] = handleTU;
  FSMhandler[7] = handleCancelQSO;


// --- define my own coordinates 

  sprintf(mycall,"%s","LU7DID");
  sprintf(mygrid,"%s","GF05");
  sprintf(mysig,"%s","10");
  sprintf(filePath,"./");

//---------------------------------------------------------------------------------
// arg_parse (parameters override previous configuration)
//---------------------------------------------------------------------------------
   while(1)
	{
	int ax = getopt(argc, argv, "f:g:v:s:l:c:r:xh");
	if(ax == -1) 
	{
	  if(anyargs) break;
	  else ax='h'; //print usage and exit
        }
	anyargs = 1;

	switch(ax)
	{
	case 'l': // log file path
             sprintf(filePath,"%s",optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....grid(%s)\n",PROGRAMID,mygrid) : _NOP);
	     break;
	case 'g': // my grid locator
             sprintf(mygrid,"%s",optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....grid(%s)\n",PROGRAMID,mygrid) : _NOP);
	     break;
	case 'c': // mycall
             sprintf(mycall,"%s",optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....call(%s)\n",PROGRAMID,mycall) : _NOP);
	     break;
	case 'f': // Frequency
	     freq = atof(optarg);
  	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....frequency(%f)\n",PROGRAMID,freq) : _NOP);
	     break;
	case 'x': // SampleRate (Only needeed in IQ mode)
	     HAL = 0x01;
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....HAL(%d)\n",PROGRAMID,HAL) : _NOP);
             break;
	case 'v': // SampleRate (Only needeed in IQ mode)
	     TRACE = atoi(optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....tracelevel(%d)\n",PROGRAMID,TRACE) : _NOP);
             break;
	case 'r': // SampleRate (Only needeed in IQ mode)
	     strcpy(mysig,optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....SNR(%d)\n",PROGRAMID,atoi(mysig)) : _NOP);
             break;
	case 's':  // SampleRate (RF sampling for FT8 decoding)
	     sr = atoi(optarg);
	     (TRACE>=0x00 ? fprintf(stderr,"%s:(arg)....samplerate(%d)\n",PROGRAMID,sr) : _NOP);
             break;

	case -1:
             break;
	case '?':
	     if (isprint(optopt) )
 	     {
 	        (TRACE>=0x00 ? fprintf(stderr, "%s:(arg)....unknown option `-%c'.\n",PROGRAMID,optopt) : _NOP);
 	     } 	else {
		(TRACE>=0x00 ?fprintf(stderr, "%s:(arg)....unknown option character `\\x%x'.\n",PROGRAMID,optopt) : _NOP);
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



// ---

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() Memory initialization\n",PROGRAMID) : _NOP);
  buffer=(char*)malloc(2048*sizeof(unsigned char));
  sprintf(inChar,"R-%s",mysig);
  strcpy(mysig,inChar);
  strcpy(inChar,"");
  
  clearmsg(ft8_msg);


  (TRACE>=0x02 ? fprintf(stderr,"%s:main() FT8 decoder initialization\n",PROGRAMID) : _NOP);
  r=new decodeFT8();  
  r->TRACE=TRACE;
  r->setFrequency(freq);
  r->setMode(MUSB);
  r->sr=sr;
  r->start();

  l=new cabrillo(NULL);
  strcpy(l->callsign,mycall);
  strcpy(l->grid,mygrid);
  l->TRACE=TRACE;
  strcpy(l->filePath,filePath);
  l->start();

  setWord(&MSW,RUN,true);
  pthread_create(&t, NULL, &read_stdin, NULL);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(getWord(MSW,RUN)==true)
  {

    int nread=r->readpipe(buffer,1024);
    if (nread>0) {
       buffer[nread]=0x00;
       char* z=strtok(buffer,"\n");
       while(z!=NULL) {
          processFrame(z,ft8_header,ft8_msg,ft8_qso);
          (TRACE>=0x03 ? fprintf(stderr,"%s:main() returned from processFrame active(%s)\n",PROGRAMID,BOOL2CHAR(ft8_header->active)) : _NOP);
          z=strtok(NULL,"\n");
       }
    }

    if (ft8_qso->FSM > 0x00) {  //hay QSO en curso
       //(TRACE>=0x00 ? fprintf(stderr,"%s:main() FSM(%d) indicates a QSO in progress\n",PROGRAMID,ft8_qso->FSM) : _NOP);
    } else {
       if (getWord(MSW,GUI)==true) {
          setWord(&MSW,GUI,false);
          if (ft8_qso->FSM == 0x00) {  // no hay QSO en curso

             if (strcmp(inChar,"C")==0) {   //GUI indicates start a CQ sequence
                (TRACE>=0x01 ? fprintf(stderr,"%s:main() GUI activity informed (%s) start CQ\n",PROGRAMID,inChar) : _NOP);
                ft8_qso->FSM=0x03;
                qsoMachine(ft8_header,ft8_msg,ft8_qso);
             }

             if (strcmp(inChar,"Z")==0) {          //GUI if there is an on-going QSO cancel it
                if (ft8_qso->FSM>0x00) {           //Is there an on-going QSO
		   ft8_qso->FSM=0x07;
                   qsoMachine(ft8_header,ft8_msg,ft8_qso);
                   (TRACE>=0x01 ? fprintf(stderr,"%s:main() Started response to cancel request\n",PROGRAMID) : _NOP);
                } else {
                   (TRACE>=0x00 ? fprintf(stderr,"%s:main() Nothing to cancel, abandon\n",PROGRAMID) : _NOP);
                }
             }

             if (strcmp(inChar,"C")!=0 && strcmp(inChar,"Z") != 0) { //some other command, likely a CQ line  
                ft8_qso->nmsg=findQSO(selectQSO(inChar),ft8_msg);
                (TRACE>=0x01 ? fprintf(stderr,"%s:main() GUI activity informed (%s)=%d\n",PROGRAMID,inChar,ft8_qso->nmsg) : _NOP);
                if (ft8_qso->nmsg>=0) {
                   (TRACE>=0x01 ? fprintf(stderr,"%s:main() Started response to CQ(%s) QSO(%d)\n",PROGRAMID,inChar,ft8_qso->nmsg) : _NOP);
                   qsoMachine(ft8_header,ft8_msg,ft8_qso);
                }
             }
          }
       }
    }
  }

// --- Normal termination kills the child first and wait for its termination

  pthread_join(t, NULL);
  t=(pthread_t)-1;

  r->stop();
  delete(r);
}
