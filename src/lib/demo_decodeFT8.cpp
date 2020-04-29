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
      int   hissnr;
      int   hisslot;
      char  mycall[16];
      char  mygrid[16];
      int   mysnr;
      int   myslot;
      int   cnt;
};
 
typedef void (*QSOCALL)(header* h,msg* m,qso* q);
QSOCALL    FSMhandler[50];

decodeFT8* r=nullptr;
struct     sigaction sigact;
char       *buffer;
byte       TRACE=0x00;
byte       MSW=0x00;
char       mycall[16];
char       mygrid[16];

char       inChar[4];
float      freq=14074000;    // operating frequency

msg        ft8_msg[50];
header     ft8_header[1];
qso        ft8_qso[1];
int        nmsg=0;
int        nCQ=0;

boolean    fthread=false;
pthread_t  t=(pthread_t)-1;
pthread_t  pift8=(pthread_t)-1;
char       pcmd[1024];
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="demo_decodeFT8";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

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
char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s)); 
}

//---------------------------------------------------------------------------------------------------
// startProcess
// thread to send information
//---------------------------------------------------------------------------------------------------
void* startProcess (void * null) {

    (TRACE>=0x02 ? fprintf(stderr,"%s::startProcess() starting thread\n",PROGRAMID) : _NOP);
    char cmd[2048];
    char buf[4096];


    sprintf(cmd,"python /home/pi/OrangeThunder/bash/turnon.py && %s && python /home/pi/OrangeThunder/bash/turnoff.py",pcmd);
    (TRACE>=0x02 ? printf("%s::startProcess() cmd(%s)\n",PROGRAMID,cmd) : _NOP);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        (TRACE>=0x02 ? printf("%s::startProcess() error opening pipe\n",PROGRAMID) : _NOP);
        fthread=false;
        pthread_exit(NULL);
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        printf("OUTPUT: %s", buf);

    }

    if(pclose(fp))  {
        (TRACE>=0x02 ? printf("%s::startProcess() command not found of exited with error status\n",PROGRAMID) : _NOP);
        fthread=false;
        pthread_exit(NULL);
    }


    (TRACE>=0x02 ? printf("%s::startProcess() thread normally terminated\n",PROGRAMID) : _NOP);
    fthread=false;
    pthread_exit(NULL);
}
// --------------------------------------------------------------------------------------------------
void sendFT8(char* cmd) {

    (TRACE >= 0x00 ? fprintf(stderr,"%s::sendFT8() cmd[%s]\n",PROGRAMID,cmd) : _NOP);
    if (fthread == true) {
       (TRACE >= 0x00 ? fprintf(stderr,"%s::sendFT8() thread already running, skip!\n",PROGRAMID) : _NOP);
       return;
    }
    fthread=true;
    sprintf(pcmd,"python /home/pi/OrangeThunder/bash/turnon.py && sudo %s && python /home/pi/OrangeThunder/bash/turnoff.py",cmd);
int rct=pthread_create(&pift8,NULL, &startProcess, NULL);
    if (rct != 0) {
       fprintf(stderr,"%s:main() thread can not be created at this point rc(%d)\n",PROGRAMID,rct);
       exit(16);
    }
// --- process being launch 


   (TRACE >= 0x00 ? fprintf(stderr,"%s::sendFT8() thread launch completed[%d]\n",PROGRAMID,rct) : _NOP);
}
// --------------------------------------------------------------------------------------------------

void handleNOP(header h,msg* m,qso q) {
    (TRACE>=0x00 ? fprintf(stderr,"%s:handleNOP() do nothing hook\n",PROGRAMID) : _NOP);
    return;
}
// --------------------------------------------------------------------------------------------------

void handleStartQSO(header* h,msg* m,qso* q) {
   (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() handle start of QSO\n",PROGRAMID) : _NOP);

   q->FSM=0x01;
   strcpy(q->hiscall,m[q->nmsg].t2);
   strcpy(q->hisgrid,m[q->nmsg].t3);
   q->hissnr=10;
   strcpy(q->mycall,mycall);
   strcpy(q->mygrid,mygrid);
   q->mysnr=0;
   q->cnt=4;
   q->hisslot=m[q->nmsg].slot;
   q->myslot=(q->hisslot + 1)%2;
   (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() QSO structure created FSM(%d) [%s %s %d] (slot %d)->[%s %s %d] (slot %d) cnt(%d)\n",PROGRAMID,q->FSM,q->hiscall,q->hisgrid,q->hissnr,q->hisslot,q->mycall,q->mygrid,q->mysnr,q->myslot,q->cnt) : _NOP);

char FT8message[128];

   sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,q->mygrid,freq,q->myslot);
   sendFT8(FT8message);
   (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() FSM(%d) CMD[%s]\n",PROGRAMID,q->FSM,FT8message) : _NOP);
   return;

}
// --------------------------------------------------------------------------------------------------
void handleReply(header* h,msg* m,qso* q) {
char FT8message[128];

    (TRACE>=0x00 ? fprintf(stderr,"%s:handleReply() FSM(%d) (%s)->(%s)\n",PROGRAMID,q->FSM,q->hiscall,q->mycall) : _NOP);
    for (int i=0;i<nmsg;i++) {

//--- message is a CQ
        if (strcmp(m[i].t1,"CQ")==0) {
           if (strcmp(m[i].t2,q->hiscall)==0) {  // keep calling CQ repeat response
              (TRACE>=0x00 ? fprintf(stderr,"%s:handleReply() FSM(%d) found (%s) still calling CQ\n",PROGRAMID,q->FSM,q->hiscall) : _NOP);
              sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,q->mygrid,freq,q->myslot);
              sendFT8(FT8message);
              (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() CMD[%s]\n",PROGRAMID,FT8message) : _NOP);
              q->cnt--;
              if (q->cnt==0) {   //give up
                 (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() retry exceeded\n",PROGRAMID) : _NOP);
                 q->FSM=0x00;
              }
              return;
           } else {
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() somebody else CQ, ignore\n",PROGRAMID) : _NOP);
           }
        }

//--- message towards me

       if (strcmp(m[i].t1,q->mycall)==0) {
          if (strcmp(m[i].t2,q->hiscall)==0) {
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() answering to me, not implemented yet\n",PROGRAMID) : _NOP);
             sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,(char*)"R-10",freq,q->myslot);
             sendFT8(FT8message);
             q->cnt=4;
             q->FSM=0x02;
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() next state(%d)\n",PROGRAMID,q->FSM) : _NOP);
             return;
          } else { //to me but not from who I'm in QSO with, so ignore it.
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() ignore message (%s %s %s)  state(%d)\n",PROGRAMID,m[i].t1,m[i].t2,m[i].t3,q->FSM) : _NOP);
             return;
          }
       }


//--- message not CQ but sent by my destination, in QSO with somebody else, abandon

       if (strcmp(m[i].t2,q->hiscall)==0) {
          if (strcmp(m[i].t1,q->mycall)==0) {          //erroneous message I'm not sending 
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() answering to somebody else, abandon\n",PROGRAMID) : _NOP);
             return;
          } else {
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() answering to somebody else, abandon\n",PROGRAMID) : _NOP);
             q->FSM=0x00;
             return;
          }
       }

// ---- Explore the rest of the QSO lines available at this point

    }

// --- all messages scanned but no process hit found, retry message for this state
     sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,q->mygrid,freq,q->myslot);
     sendFT8(FT8message);
     (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() CMD[%s]\n",PROGRAMID,FT8message) : _NOP);
     q->cnt--;
     if (q->cnt==0) {   //give up
        (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() retry exceeded\n",PROGRAMID) : _NOP);
        q->FSM=0x00;
        return;
     }

     return;

}
// --------------------------------------------------------------------------------------------------
void handleRRR(header* h,msg* m,qso* q) {

char FT8message[128];

    (TRACE>=0x00 ? fprintf(stderr,"%s:handleReply() FSM(%d) (%s)->(%s)\n",PROGRAMID,q->FSM,q->hiscall,q->mycall) : _NOP);
    for (int i=0;i<nmsg;i++) {

//--- message is a CQ
        if (strcmp(m[i].t1,"CQ")==0) {
           (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() somebody else CQ, ignore\n",PROGRAMID) : _NOP);
        }

//--- message towards me

       if (strcmp(m[i].t1,q->mycall)==0) {
          if (strcmp(m[i].t2,q->hiscall)==0) {  // could be either a repeat of the signal using R-00 or a goodby [RRR,73,R73]
             if(strstr(m[i].t3, "73") != NULL) {
               (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() some 73 detected [%s]\n",PROGRAMID,m[i].t3) : _NOP);
               sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,"RR73",freq,q->myslot);
               sendFT8(FT8message);
               q->FSM=0x00;
               (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() completed QSO sequence [%s]\n",PROGRAMID,FT8message) : _NOP);
               return;
             } else {
               (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() some report repeat [%s]\n",PROGRAMID,m[i].t3) : _NOP);
               sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,"R-10",freq,q->myslot);
               sendFT8(FT8message);
               (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() repeat completed [%s]\n",PROGRAMID,FT8message) : _NOP);
               q->cnt--;
               if (q->cnt==0) {
                  (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() retry exceeded, abandon [%s]\n",PROGRAMID,FT8message) : _NOP);
                  q->FSM=0x00;
                  return;
               }
             }
         } else {
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() answering to me, ignore\n",PROGRAMID) : _NOP);
             return;
         }

      }
//--- message not CQ but sent by my destination, in QSO with somebody else, abandon

       if (strcmp(m[i].t2,q->hiscall)==0) {
          if (strcmp(m[i].t1,q->mycall)!=0) {          //erroneous message I'm not sending 
             (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() answering to somebody else, abandon\n",PROGRAMID) : _NOP);
             q->FSM=0x00;
             return;
          }
       }

// ---- Explore the rest of the QSO lines available at this point but ignore all of them

    }

// --- all messages scanned but no process hit found, retry message for this state
     sprintf(FT8message,"pift8 -m \"%s %s %s\" -f %f -s %d",q->hiscall,q->mycall,"R-10",freq,q->myslot);
     sendFT8(FT8message);
     (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() CMD[%s]\n",PROGRAMID,FT8message) : _NOP);
     q->cnt--;
     if (q->cnt==0) {   //give up
        (TRACE>=0x00 ? fprintf(stderr,"%s:handleStart() retry exceeded\n",PROGRAMID) : _NOP);
        q->FSM=0x00;
        return;
     }

     return;

}
// --------------------------------------------------------------------------------------------------

void handleEXCH(header* h,msg* m,qso* q) {
    (TRACE>=0x00 ? fprintf(stderr,"%s:handleEXCH() handle EXCH\n",PROGRAMID) : _NOP);
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
       if (FSMhandler[q->FSM] != nullptr) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:qsoMachine() service handler found for FSM state(%d) calling\n",PROGRAMID,q->FSM) : _NOP);
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
       (TRACE>=0x03 ? fprintf(stderr,"%s:selectQSO() option selected[%d]\n",PROGRAMID,r) : _NOP);
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

       (TRACE>=0x02 ? fprintf(stderr,"%s:read_stdin() Keyboard thread started\n",PROGRAMID) : _NOP);

       /*
        * ioctl() would be better here; only lazy
        * programmers do it this way:
        */
        system("/bin/stty cbreak");        /* reacts to Ctl-C */
        system("/bin/stty -echo");         /* no echo */
        while (getWord(MSW,RUN)==true) {
          c = getchar();
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
//*----------------------------------------------------------------------------------------------------------------
//* clear all messages from the previous cycle
//* messages must be available during most of the next cycle till a new decode message is received
//*----------------------------------------------------------------------------------------------------------------
void clearmsg(msg *m) {

  for (int i=0;i<50;i++) {
    sprintf(m[i].timestamp,"%s","");
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
//*----------------------------------------------------------------------------------------------------------------
//* parse slot header information
//*----------------------------------------------------------------------------------------------------------------
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

       fprintf(stderr,"\033[0;31m----[%s] (seq=%d slot=%d) (%d/%d) (%03d- %02d) active(%s)\033[0m\n",(char*)h->timestamp,seq,h->slot,status,brk,candidates,decoded,BOOL2CHAR(h->active));

}
//*----------------------------------------------------------------------------------------------------------------
//* parse message information
//*----------------------------------------------------------------------------------------------------------------
int parseMessage(char* st,header* h,int j,msg* m,qso* q) {

char * p;
char * x;
char    aux[32];
msg    f;


       (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage msg(%d) so far\n",PROGRAMID,j) : _NOP);

       p=strtok(st,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage wrong format\n",PROGRAMID) : _NOP );
          return j;
       }

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage quite wrong format\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.timestamp,"%s",p);  //timeMsg

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage incomplete message\n",PROGRAMID) : _NOP );
          return j;
       }

       sprintf(aux,"%s",p);
       f.snr=atoi(trim(aux));
       if (f.snr==-1) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage last message FSM(%d)\n",PROGRAMID,q->FSM) : _NOP );
          return -1;
       }

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage corrupted message\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(aux,"%s",p);
       
       f.DT=atof(trim(aux));

       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage partial message not processed\n",PROGRAMID) : _NOP );
          return j;
       }

       sprintf(aux,"%s",p);
       f.offset=atoi(trim(aux));


       p=strtok(NULL,",");
       if (p==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage parsing error (t1)\n",PROGRAMID) : _NOP );
          return j;
       }
char   mssg[128];
       sprintf(mssg,"%s",p);

       x=strtok(mssg," ");
       if (x==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage parsing error (t2)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t1,"%s",x);

       x=strtok(NULL," ");
       if (x==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage parsing error (t3)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t2,"%s",x);

       x=strtok(NULL," ");
       if (x==NULL) {
          (TRACE>=0x03 ? fprintf(stderr,"%s:processMessage parsing error (t3)\n",PROGRAMID) : _NOP );
          return j;
       }
       sprintf(f.t3,"%s",x);
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseMessage()  %s %d %f %d %s %s %s\n",PROGRAMID,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3) : _NOP);


       strcpy(m[j].timestamp,f.timestamp);
       m[j].snr=f.snr;
       m[j].DT=f.DT;
       m[j].offset=f.offset;
       m[j].slot=h->slot;
       strcpy(m[j].t1,f.t1);
       strcpy(m[j].t2,f.t2);
       strcpy(m[j].t3,f.t3);
       m[j].CQ=false;

       if (strcmp("CQ",f.t1)==0) {
          m[j].CQ=true;
          fprintf(stderr,"\033[1;33m[%02d] %s %4d %4.1f %04d %s %s %s\033[0m\n",nCQ,f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          nCQ++;
          j++;
          return j;
       }

       if (strcmp(mycall,f.t1)==0) {
          j++;
          fprintf(stderr,"\033[1;33m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
          return j;
       }

       fprintf(stderr,"\033[0;32m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timestamp,f.snr,f.DT,f.offset,f.t1,f.t2,f.t3);
       j++;
       return j;
}

// ======================================================================================================================
// processFrame
// manipulate received frames to operate FT8
//=======================================================================================================================
void processFrame(char* s,header* h,msg* m,qso* q) {

char * p;
char d[256];

      (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() processing frame(%s)\n",PROGRAMID,s) : _NOP ); 
   
      if (s==NULL) {
         (TRACE>=0x00 ? fprintf(stderr,"%s:processFrame() empty frame received, ignored\n",PROGRAMID) : _NOP ); 
         return;
      }


      if (strstr(s,"HDR,") != NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() hdr message(%s)\n",PROGRAMID,s) : _NOP);
         parseHeader(s,h,q);
         h->active=true;
         h->newheader=true;
         (TRACE>=0x02 ? fprintf(stderr,"%s:processFrame() hdr initialized new header(%s)\n",PROGRAMID,BOOL2CHAR(h->newheader)) : _NOP);
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
         if (n==-1) {
            if (q->FSM >= 0x01) {       // there is a QSO in progress
               qsoMachine(h,m,q);
            }          
         } else {
           nmsg=n;
         }
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() number of messages(%d)\n",PROGRAMID,nmsg) : _NOP);

         return; 
      } 

      fprintf(stderr,"\033[0;32m%s\033[0m\n",s);
      return;
 
}
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
  //FSMhandler[3] = handleRRR;


// --- define my own coordinates 

  sprintf(mycall,"%s","LU7DID");
  sprintf(mygrid,"%s","GF05");

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() Memory initialization\n",PROGRAMID) : _NOP);
  buffer=(char*)malloc(2048*sizeof(unsigned char));

  clearmsg(ft8_msg);


  (TRACE>=0x02 ? fprintf(stderr,"%s:main() FT8 decoder initialization\n",PROGRAMID) : _NOP);
  r=new decodeFT8();  
  r->TRACE=TRACE;
  r->setFrequency(freq);
  r->setMode(MUSB);
  r->sr=12000;
  r->start();

  //gpio_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));
// --- gpio Wrapper creation

  //(TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize gpio Wrapper\n",PROGRAMID) : _NOP);
  //g=new gpioWrapper(NULL);
  //g->TRACE=TRACE;

  //if (g->setPin(GPIO_PTT,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
  //   (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_PTT) : _NOP);
  //   exit(16);
  //}
  //if (g->setPin(GPIO_PA,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
  //   (TRACE>=0x0 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_PA) : _NOP);
  //   exit(16);
  //}
  //if (g->setPin(GPIO_AUX,GPIO_IN,GPIO_PUP,GPIO_NLP) == -1) {
  //   (TRACE>=0x0 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_AUX) : _NOP);
  //   exit(16);
  //}
  //if (g->setPin(GPIO_KEYER,GPIO_IN,GPIO_PUP,GPIO_NLP) == -1) {
  //   (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_KEYER) : _NOP);
  //   exit(16);
  //}

  //if (g->setPin(GPIO_COOLER,GPIO_OUT,GPIO_PUP,GPIO_NLP) == -1) {
  //   (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to initialize pin(%s)\n",PROGRAMID,(char*)GPIO_KEYER) : _NOP);
  //   exit(16);
  //}

  //if (g->start() == -1) {
  //   (TRACE>=0x00 ? fprintf(stderr,"%s:main() failure to start gpioWrapper object\n",PROGRAMID) : _NOP);
  //   exit(8);
  //}

  // *---------------------------------------------*
  // * Set cooler ON mode                          *
  // *---------------------------------------------*
  //(TRACE>=0x01 ? fprintf(stderr,"%s:main() operating relay to cooler activation\n",PROGRAMID) : _NOP);
  //if(g!=nullptr) {g->writePin(GPIO_COOLER,1);}
  //usleep(10000);

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
          (TRACE>=0x01 ? fprintf(stderr,"%s:main() returned from processFrame active(%s)\n",PROGRAMID,BOOL2CHAR(ft8_header->active)) : _NOP);

          z=strtok(NULL,"\n");
       }
    }

    if (getWord(MSW,GUI)==true) {
       setWord(&MSW,GUI,false);
       if (ft8_qso->FSM == 0x00) {  // hay QSO en curso
          ft8_qso->nmsg=findQSO(selectQSO(inChar),ft8_msg);
          (TRACE>=0x01 ? fprintf(stderr,"%s:main() GUI activity informed (%s)=%d\n",PROGRAMID,inChar,ft8_qso->nmsg) : _NOP);
          if (ft8_qso->nmsg>=0) {
             (TRACE>=0x01 ? fprintf(stderr,"%s:main() Started response to CQ(%s) QSO(%d)\n",PROGRAMID,inChar,ft8_qso->nmsg) : _NOP);
             qsoMachine(ft8_header,ft8_msg,ft8_qso);
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
