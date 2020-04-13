/*
 * rtl-fm, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 *-----------------------------------------------------------------------------


 * Quick refactoring to integrate origina rtl_fm.c code into OT project
 * main reasons for change:
 *    - introduce ability to change frequency while running
 *    - introduce ability to change mode while running
 *    - introduce functionality to mute the receiver (while transmiting)
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
#include "/home/pi/OrangeThunder/src/lib/rtlfm.h"

// --- IPC structures


int    bRun=0;
rtlfm* r=nullptr;
struct sigaction sigact;
char   *buffer;
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
	fprintf(stderr, "\nSignal caught(%d), exiting!\n",signum);
	bRun = 1;
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


  buffer=(char*)malloc(2048*sizeof(unsigned char));

  r=new rtlfm();  
  r->start();
  r->setFrequency(7074000);
  r->setFrequency(14074000);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(bRun==0)
  {
    int nread=r->readpipe(buffer,1024);
    if (nread>0) {
       buffer[nread]=0x00;
       fprintf(stderr,"%s",(char*)buffer);
    }
  }

// --- Normal termination kills the child first and wait for its termination

  r->stop();

}
