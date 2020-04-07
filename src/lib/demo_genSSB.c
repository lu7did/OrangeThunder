/*
 * demo_genSSB
 *-----------------------------------------------------------------------------
 * demo program for the genSSB program component
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
#include "/home/pi/OrangeThunder/src/lib/genSSB.h"

// --- IPC structures


int    bRun=0;
genSSB* g=nullptr;
struct sigaction sigact;
char   *buffer;


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

  g=new genSSB();  
  g->start();
  g->setFrequency(7074000);
  g->setFrequency(14074000);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(bRun==0)
  
  {
   
    int nread=g->readpipe(buffer,1024);
    buffer[nread]=0x00;
    fprintf(stderr,"%s",(char*)buffer);
    //write(STDOUT_FILENO,buffer,nread);
        
  }

// --- Normal termination kills the child first and wait for its termination

  g->stop();

}