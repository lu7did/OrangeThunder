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

// --- IPC structures



int   bRun=0;
pid_t pid = 0;
int   inpipefd[2];
int   outpipefd[2];

char  buffer[2048];
char  msg[256];
int   status;


struct sigaction sigact;
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


// --- create pipes

  pipe(inpipefd);
  fcntl(inpipefd[1],F_SETFL,O_NONBLOCK);

  pipe(outpipefd);
  fcntl(outpipefd[0],F_SETFL,O_NONBLOCK);

// --- launch pipe

  pid = fork();

  if (pid == 0)
  {

// --- This is executed by the child only, output is being redirected

    dup2(outpipefd[0], STDIN_FILENO);
    dup2(inpipefd[1], STDOUT_FILENO);
    dup2(inpipefd[1], STDERR_FILENO);

// --- ask kernel to deliver SIGTERM in case the parent dies

    prctl(PR_SET_PDEATHSIG, SIGTERM);

// --- process being launch, which is a test conduit of rtl_fm, final version should have some fancy parameterization

    execl(getenv("SHELL"),"sh","-c","sudo /home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14.074M -s 4000 -E direct | mplayer -nocache -af volume=-10 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -",NULL);

// --- Nothing below this line should be executed by child process. If so, 
// --- it means that the execl function wasn't successfull, so lets exit:
    exit(1);
  }

//**************************************************************************
// The code below will be executed only by parent. You can write and read
// from the child using pipefd descriptors, and you can send signals to 
// the process using its pid by kill() function. If the child process will
// exit unexpectedly, the parent process will obtain SIGCHLD signal that
// can be handled (e.g. you can respawn the child process).
//**************************************************************************
// --- close unused pipe ends

  close(outpipefd[0]);
  close(inpipefd[1]);

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(bRun==0)
  
  {
    int nread=read(inpipefd[0],buffer,1024);
    buffer[nread]=0x00;
    fprintf(stderr,"%s",(char*)buffer);
    //write(STDOUT_FILENO,buffer,nread);
        
  }

// --- Normal termination kills the child first and wait for its termination

  fprintf(stderr,"Killing child process pid(%d)\n",pid);
  kill(pid, SIGKILL); //send SIGKILL signal to the child process
  waitpid(pid, &status, 0);
  fprintf(stderr,"Successfully completed!\n");
}
