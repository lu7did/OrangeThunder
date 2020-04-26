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
#include <ncurses.h>


#include "/home/pi/OrangeThunder/src/lib/decodeFT8.h"
#include "/home/pi/OrangeThunder/src/lib/gpioWrapper.h"


// --- IPC structures

struct FT8msg {
       char timeMsg[16];    // time stamp
       byte slot;           //
       byte odd;            // 
        int db;             // Signal SNR
      float DT;             // Time Shift
        int offset;         // Frequency offset
       char t1[16];         // first token
       char t2[16];         // second token
       char t3[16];         // third token
};

struct FT8slot {
      char  timeSlot[16];
      byte  slot;
      byte  odd;
      byte  miss;
      byte  brk;
      int   candidates;
      int   decoded;
};
 
struct FT8qso {
      byte  FSM;
      byte  cnt;
      byte  urslot;
      byte  myslot;
      byte  urodd;
      byte  myodd;
      char  timeQSO[16];
      char  urcall[16];
      char  urgrid[16];
      int   myRST;
      int  urRST;
};

decodeFT8* r=nullptr;

struct     sigaction sigact;
char       *buffer;
byte       TRACE=0x00;
byte       MSW=0x00;
char       mycall[16];
char       mygrid[16];
FT8qso     qso;
FT8slot    s;

char       inChar[4];

FT8msg     ft8[50];
boolean    fQSO=false;
int        nQSO=0;
boolean    fSlot=false;
//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="demo_decodeFT8";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

struct termios orig_termios;
// *----------------------------------------------------------------*
// *                  GPIO support processing                       *
// *----------------------------------------------------------------*
// --- gpio object
gpioWrapper* g=nullptr;
char   *gpio_buffer;
void gpiochangePin();

// *----------------------------------------------------------------*
// *           Read commans from Std input while running            *
// *----------------------------------------------------------------*
pthread_t t=(pthread_t)-1;
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
// Process QSO
//--------------------------------------------------------------------------------------------------
void selectQSO(char* c) {

int q=atoi(c);

    (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() QSO[%d]\n",PROGRAMID,q) : _NOP);

    if (nQSO==0) {
       (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() QSO[%d] not available\n",PROGRAMID,q) : _NOP);
       return;
    }

    if (q>=0 && q<nQSO) {
       int myodd=(ft8[q].odd+1)%2;
       (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() {urOdd=%d->myOdd=%d} [%s %03d %4.1f %4d %s %s %s]\n",PROGRAMID,ft8[q].odd,myodd,ft8[q].timeMsg,ft8[q].db,ft8[q].DT,ft8[q].offset,ft8[q].t1,ft8[q].t2,ft8[q].t3) : _NOP);

    } else {
       (TRACE>=0x00 ? fprintf(stderr,"%s:selectQSO() QSO[%d] out of range\n",PROGRAMID,q) : _NOP);
       return;
    }

 
}
//--------------------------------------------------------------------------------------------------
// read_stdin without waiting for an enter to be pressed
//--------------------------------------------------------------------------------------------------
void* read_stdin(void * null) {

int  c;
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
          sprintf(buf,"%c",c);
          selectQSO(buf);
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
//*----------------------------------------------------------------------------------------------------------------
//* erase message structure
//*----------------------------------------------------------------------------------------------------------------
void clearmsg() {

  for (int i=0;i<50;i++) {
    sprintf(ft8[i].timeMsg,"%s","");
    ft8[i].slot=0;
    ft8[i].odd=0;
    ft8[i].db=0;
    ft8[i].DT=0.0;
    ft8[i].offset=0;
    sprintf(ft8[i].t1,"%s","");
    sprintf(ft8[i].t2,"%s","");
    sprintf(ft8[i].t3,"%s","");
  }

}
//*----------------------------------------------------------------------------------------------------------------
//* parse slot header information
//*----------------------------------------------------------------------------------------------------------------
void parseHeader(char* st) {

char * p;
char   aux[32];

       p=strtok(st," ");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:parseHeader() malformed header, ignored\n",PROGRAMID) : _NOP);
          return;
       }

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
      
       sprintf(aux,"%s",p);  //timeMsg
       strcpy(s.timeSlot,aux);


       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       s.slot=atoi(aux);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (strcmp(aux,"e")==0 ? s.odd = 0 : s.odd = 1);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (strcmp(aux,"OK")==0 ? s.miss = 0 : s.miss = 1);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       (strcmp(aux,"cpl")==0 ? s.brk = 0 : s.brk = 1);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       s.candidates=atoi(aux);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);  //timeMsg
       s.decoded=atoi(aux);


       fprintf(stderr,"\033[0;31m----[%s] (slot=%d seq=%d) (%d/%d) (%03d- %02d)\033[0m\n",(char*)s.timeSlot,s.slot,s.odd,s.miss,s.brk,s.candidates,s.decoded);

}

//*----------------------------------------------------------------------------------------------------------------
//* processQSO
//*----------------------------------------------------------------------------------------------------------------
int processQSO(FT8msg* ft8) {


    switch(qso.FSM) {

       case 0x00 :  {

                    break;
                    }

       case 0x01 :  {


                    break;
                    }

       case 0x02:   {


                    break;
                    }
       case 0x03:   {
         
                    break;
                    }


    }

    return  0;
}
//*----------------------------------------------------------------------------------------------------------------
//* parse message information
//*----------------------------------------------------------------------------------------------------------------
void parseMessage(char* st) {

char * p;
char    aux[32];
FT8msg  f;

       p=strtok(st," ");
       if (p==NULL) {
          return;
       }

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(f.timeMsg,"%s",p);  //timeMsg

       p=strtok(NULL," ");
       if (p==NULL) {
          (TRACE>=0x00 ? fprintf(stderr,"%s:processMessage detected last message\n",PROGRAMID) : _NOP );
          return;
       }

       sprintf(aux,"%s",p);
       f.db=atoi(aux);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(aux,"%s",p);
       f.DT=atof(aux);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }

       sprintf(aux,"%s",p);
       f.offset=atoi(aux);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(f.t1,"%s",p);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(f.t2,"%s",p);

       p=strtok(NULL," ");
       if (p==NULL) {
          return;
       }
       sprintf(f.t3,"%s",p);
       (TRACE>=0x03 ? fprintf(stderr,"%s:parseMessage()  %s %d %f %d %s %s %s\n",PROGRAMID,f.timeMsg,f.db,f.DT,f.offset,f.t1,f.t2,f.t3) : _NOP);

       if (strcmp("CQ",f.t1)==0) {
          if (qso.FSM==0x00 ) {
             strcpy(ft8[nQSO].timeMsg,f.timeMsg);
             ft8[nQSO].db=f.db;
             ft8[nQSO].DT=f.DT;
             ft8[nQSO].slot=s.slot;
             ft8[nQSO].odd=s.odd;
             ft8[nQSO].offset=f.offset;
             strcpy(ft8[nQSO].t1,f.t1);
             strcpy(ft8[nQSO].t2,f.t2);
             strcpy(ft8[nQSO].t3,f.t3);
             fQSO=true;
             fprintf(stderr,"\033[1;33m[%02d] %s %4d %4.1f %04d %s %s %s\033[0m\n",nQSO,f.timeMsg,f.db,f.DT,f.offset,f.t1,f.t2,f.t3);
             nQSO++;
             return;
       } else {
             fprintf(stderr,"\033[0;33m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timeMsg,f.db,f.DT,f.offset,f.t1,f.t2,f.t3);
             return;
       }}

       if (strcmp(mycall,f.t1)==0) {
          fprintf(stderr,"\033[1;33m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timeMsg,f.db,f.DT,f.offset,f.t1,f.t2,f.t3);
          return;
       }

       fprintf(stderr,"\033[0;32m     %s %4d %4.1f %04d %s %s %s\033[0m\n",f.timeMsg,f.db,f.DT,f.offset,f.t1,f.t2,f.t3);
       return;
}

// ======================================================================================================================
// processFrame
// manipulate received frames to operate FT8
//=======================================================================================================================
void processFrame(char* s) {


char d[256];

      if (s==NULL) {
         (TRACE>=0x02 ? fprintf(stderr,"%s:processFrame() empty frame received, ignored\n",PROGRAMID) : _NOP ); 
         return;
      }

      s[strlen(s)-1] = 0x00;
      sprintf(d,"%s",s);
      if (strstr(s,"decodeFT8:FT8<loop>") != NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() loop message(%s)\n",PROGRAMID,d) : _NOP);
         fSlot=true;
         parseHeader(d);
         return;
      }

      if (strstr(s,"decodeFT8:FT8<mssg>") != NULL) {
         (TRACE>=0x03 ? fprintf(stderr,"%s:processFrame() mssg message(%s)\n",PROGRAMID,d) : _NOP);
         if(fSlot==true) {
           fSlot=false;
           clearmsg();
           nQSO=0;
           fQSO=false;
         }
         parseMessage(d);
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

//  set_conio_terminal_mode();


  sprintf(mycall,"%s","LU7DID");
  sprintf(mygrid,"%s","GF05");

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() Memory initialization\n",PROGRAMID) : _NOP);
  buffer=(char*)malloc(2048*sizeof(unsigned char));

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() QSO structure initialization\n",PROGRAMID) : _NOP);
  qso.FSM=0x00;
  qso.cnt=0x00;
  qso.urslot=0x00;
  qso.myslot=0x00;
  qso.urodd=0x00;
  qso.myodd=0x00;
  sprintf(qso.timeQSO,"%s","");
  sprintf(qso.urcall,"%s","");
  sprintf(qso.urgrid,"%s","");
  qso.myRST=-10;
  qso.urRST=-10;

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() time slot structure initialization\n",PROGRAMID) : _NOP);

  sprintf(s.timeSlot,"%s","");
  s.slot=0;
  s.odd=0;
  s.miss=0;
  s.brk=0;
  s.candidates=0;
  s.decoded=0;

  (TRACE>=0x02 ? fprintf(stderr,"%s:main() payload structure initialization\n",PROGRAMID) : _NOP);

  clearmsg();


  (TRACE>=0x02 ? fprintf(stderr,"%s:main() FT8 decoder initialization\n",PROGRAMID) : _NOP);
  r=new decodeFT8();  
  r->TRACE=TRACE;
  r->setFrequency(14074000);
  r->setMode(MUSB);
  r->sr=12000;
  r->start();


  gpio_buffer=(char*)malloc(GENSIZE*sizeof(unsigned char));
// --- gpio Wrapper creation

  (TRACE>=0x01 ? fprintf(stderr,"%s:main() initialize gpio Wrapper\n",PROGRAMID) : _NOP);
  g=new gpioWrapper(NULL);
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

  // *---------------------------------------------*
  // * Set cooler ON mode                          *
  // *---------------------------------------------*
  (TRACE>=0x01 ? fprintf(stderr,"%s:main() operating relay to cooler activation\n",PROGRAMID) : _NOP);
  if(g!=nullptr) {g->writePin(GPIO_COOLER,1);}
  usleep(10000);

  setWord(&MSW,RUN,true);

  pthread_create(&t, NULL, &read_stdin, NULL);




// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(getWord(MSW,RUN)==true)
  {
    int nread=r->readpipe(buffer,1024);
    if (nread>0) {
       buffer[nread]=0x00;
       processFrame((char*)buffer);
    }

  }

// --- Normal termination kills the child first and wait for its termination

  pthread_join(t, NULL);
  t=(pthread_t)-1;
  r->stop();
  delete(r);
}
