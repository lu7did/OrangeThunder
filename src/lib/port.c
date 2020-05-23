#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

int main(int argc, char *argv[])
{

char buffer[256];
int  pin=12;
int  v=0;

     sprintf(buffer,"python /home/pi/OrangeThunder/python/gpioset.py %d %d",pin,v);
int  rc=system(buffer);
     fprintf(stderr,"Sent command to execute(%s) rc(%d)\n",buffer,rc);
     exit(0);
}
