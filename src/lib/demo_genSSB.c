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
#include <pigpio.h>
#include "/home/pi/OrangeThunder/src/lib/genSSB.h"
#include "/home/pi/PixiePi/src/lib/RPI.h" 

#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))



// --- IPC structures


int    bRun=0;
genSSB* g=nullptr;
struct sigaction sigact;
char   *buffer;
char   *HW;

const char   *PROGRAMID="demo_genSSB";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";

// ======================================================================================================================
// sighandler
// ======================================================================================================================
static void sighandler(int signum)
{
	fprintf(stderr, "\nSignal caught(%d), exiting!\n",signum);
	bRun = 1;
}
//--------------------------------------------------------------------------------------------------
// map_peripheral
// Exposes the physical address defined in the passed structure using mmap on /dev/mem
//--------------------------------------------------------------------------------------------------
int map_peripheral(struct bcm2835_peripheral *p)
{
   // Open /dev/mem
   if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("Failed to open /dev/mem, try checking permissions.\n");
      return -1;
   }
 
   p->map = mmap(
      NULL,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,      // File descriptor to physical memory virtual file '/dev/mem'
      p->addr_p       // Address in physical map that we want this memory block to expose
   );
 
   if (p->map == MAP_FAILED) {
        perror("mmap");
        return -1;
   }
 
   p->addr = (volatile unsigned int *)p->map;
 
   return 0;
}
//--------------------------------------------------------------------------------------------------
// unmap_peripheral
// release resources
//--------------------------------------------------------------------------------------------------
void unmap_peripheral(struct bcm2835_peripheral *p) {
 
    munmap(p->map, BLOCK_SIZE);
    close(p->mem_fd);
}

//--------------------------------------------------------------------------------------------------
// setGPIO
// output status of a given GPIO pin
//--------------------------------------------------------------------------------------------------
void setGPIO(int pin,bool v) {

// ---  acquire resources
 return;

 if(map_peripheral(&gpio) == -1) 
  {
    fprintf(stderr,"Failed to map the physical GPIO registers into the virtual memory space.\n");
    return ;
  }
// --- Clear pin definition and then set as output

  INP_GPIO(pin);
  OUT_GPIO(pin);

// --- Map result to pin
 
  if (v==true) {
    GPIO_SET = 1 << pin;  // Set as high
  } else {
    GPIO_CLR = 1 << pin;  // Set as low
  }

// --- release resources

  unmap_peripheral(&gpio);
  return; 
}
// ======================================================================================================================
// PTT change
// ======================================================================================================================
void SSBchangeVOX() {

  char command[256];
  fprintf(stderr,"%s:SSBchangeVOX() received upcall from genSSB object state(%s)\n",PROGRAMID,(g->stateVOX==true ? "True" : "False"));
  g->setPTT(g->stateVOX);

  if (g->stateVOX==true) {  
     gpioWrite(12, 1);
     usleep(10000);

  } else {
     gpioWrite(12,0);
     usleep(10000);

  }
  //setGPIO(12,g->stateVOX);
  fprintf(stderr,"%s:SSBchangeVOX() set PTT as(%s)\n",PROGRAMID,(g->statePTT==true ? "True" : "False"));


// --- process being launch, which is a test conduit of rtl_fm, final version should have some fancy parameterization


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

  if(gpioInitialise()<0) {
    fprintf(stderr,"%s:main Cannot initialize GPIO",PROGRAMID);
    return -1;
  }

  gpioSetMode(12, PI_OUTPUT);
  usleep(100000);

  gpioSetPullUpDown(12,PI_PUD_UP);
  usleep(100000);

  gpioWrite(12, 0);
  usleep(100000);



  buffer=(char*)malloc(2048*sizeof(unsigned char));
  HW=(char*)malloc(16*sizeof(unsigned char));
  sprintf(HW,"Loopback");

  g=new genSSB(SSBchangeVOX);  
  g->TRACE=0x02;
  g->setFrequency(7074000);
  g->setFrequency(14074000);
  g->setSoundChannel(1);
  g->setSoundSR(48000);
  g->setSoundHW(HW);

  g->start();

// --- Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(bRun==0)
  
  {
   
    int nread=g->readpipe(buffer,1024);
    buffer[nread]=0x00;
    fprintf(stderr,"%s",(char*)buffer);
  }

// --- Normal termination kills the child first and wait for its termination

  g->stop();

}
