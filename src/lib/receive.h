//--------------------------------------------------------------------------------------------------
// reeive SSB receiver   (HEADER CLASS)
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// receive class implementation of a simple USB receiver
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

#ifndef receive_h
#define receive_h

#define _NOP        (byte)0

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sched.h>
#include <math.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>


#include "/home/pi/csdr/libcsdr.h"
#include "/home/pi/csdr/libcsdr_gpl.h"
#include "/home/pi/csdr/ima_adpcm.h"
#include "/home/pi/csdr/fastddc.h"


#define MAXLEVEL 1

//----- System Variables

#define BUFFERSIZE 96000
#define IQBURST          4000
#define BUFSIZE    2048*16
//---------------------------------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------------------------------

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();
typedef struct shift_addition_cc_s
{
    float  starting_phase=0;
    float  rate;
    int    fd;
    int    remain;
    int    current_size;
    float* ibufptr;
    float* obufptr;
    shift_addition_data_t data;

} shift_addition_cc_t;


typedef struct rtl_sdr_s
{
    float   SetFrequency;         
    float   LO;
    float   SR;
} rtl_sdr_t;

void setWord(unsigned char* SysWord,unsigned char  v, bool val);
bool getWord (unsigned char SysWord, unsigned char v);

//---------------------------------------------------------------------------------------------------
// SSB CLASS
//---------------------------------------------------------------------------------------------------
class receive
{
  public: 
  
      receive();
      int decode(unsigned char* buffer_u8,int the_bufsize,short *buffer_i16);

      int                 bigbufs;
      shift_addition_cc_t s;
      rtl_sdr_t           r;
      byte                TRACE=0x00;



//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="receive";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


private:

      char   msg[80]; 
      float  *input_buffer;
      float* output_buffer;

};

#endif
//---------------------------------------------------------------------------------------------------
// SSB CLASS Implementation
//--------------------------------------------------------------------------------------------------
// Este es el firmware del diseÃ±o de SSB para PixiePi
// Solo para uso de radioaficionados, prohibido su utilizacion comercial
// Copyright 2018 Dr. Pedro E. Colla (LU7DID)
//--------------------------------------------------------------------------------------------------

receive::receive()
{


// init structures for shift_addition_cc

   bigbufs=1;
   s.starting_phase=0;

   r.SetFrequency=14074000;
   r.LO=14100000;
   r.SR=1200000;

   s.rate=(r.LO-r.SetFrequency)/r.SR;

   s.data=shift_addition_init(s.rate);
   s.remain=0;
   s.current_size=0;

// allocate memory buffers

   input_buffer  = (float*)  malloc(BUFSIZE*sizeof(float) * 2);
   output_buffer = (float*)  malloc(BUFSIZE*sizeof(float) * 2);

   fprintf(stderr,"%s::receive() Object creation completed\n",PROGRAMID);
}
//*--------------------------------------------------------------------------------------------------
// decimate: take one sample and compute one filterd output
//*--------------------------------------------------------------------------------------------------
int receive::decode(unsigned char* buffer_u8,int the_bufsize,short *buffer_i16) {

// --- convert_u8_f(baseband_buffer, baseband_output, baseband_len);
    for(int i=0;i<the_bufsize;i++) {
        input_buffer[i]=((float)buffer_u8[i])/(UCHAR_MAX/2.0)-1.0; //@convert_u8_f
    }

// --- shift_addition_cc {LO-f)/SR   shift to lower baseband only the segment needed

    s.remain=the_bufsize;
    s.ibufptr=input_buffer;
    s.obufptr=output_buffer;
    while(s.remain) {
       s.current_size=(s.remain>2048)?2048:s.remain;
       s.starting_phase=shift_addition_cc((complexf*)s.ibufptr, (complexf*)s.obufptr, s.current_size, s.data, s.starting_phase);
       s.ibufptr+=s.current_size*2;
       s.obufptr+=s.current_size*2;
       s.remain-=s.current_size;
    }

    return 0;    //<=== RETURN TRUE NUMBER OF SAMPLES PROCESSED HERE

}
