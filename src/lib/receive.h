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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <pigpio.h>
#include <unistd.h>
#include <limits.h>

#define MAXLEVEL 1

//----- System Variables

#define BUFFERSIZE 96000
#define IQBURST          4000

//---------------------------------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------------------------------

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

void setWord(unsigned char* SysWord,unsigned char  v, bool val);
bool getWord (unsigned char SysWord, unsigned char v);

//---------------------------------------------------------------------------------------------------
// SSB CLASS
//---------------------------------------------------------------------------------------------------
class receive
{
  public: 
  
      receive();
      byte TRACE=0x00;
      int decode(unsigned char* baseband_buffer,int baseband_len,short *audio_buffer);
      float* baseband_output;
      float SetFrequency;
      float SampleRate;
      float LO;
      //ACB  agc;
      //int  decimation_factor;
      //int generate(short *input,int len,float* I,float* Q);




//-------------------- GLOBAL VARIABLES ----------------------------
const char   *PROGRAMID="receive";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";


private:

        char   msg[80]; 
        int bigbufs;
        float starting_phase;
        float rate;
        int decimation;
        shift_addition_data_t d;
        decimating_shift_addition_status_t s;
        float starting_phase;
        int fd;
        int remain;
        int current_size;
        float* ibufptr;
        float* obufptr;
        float* output_buffer;
        shift_addition_data_t data;

        int factor;
        float transition_bw;
        float transition_bw_BPF;

        window_t window;
        window_t window_BPF;

        int taps_length;
        int taps_length_BPF;


        int padded_taps_length;
        float *taps;

        int input_skip;
        int output_size;

        float low_cut;
        float high_cut;
        float* input_FIR;
        float* output_FIR;
    //prepare making the filter and doing FFT on it
    complexf* taps=(complexf*)calloc(sizeof(complexf),fft_size); //initialize to zero
    complexf* taps_fft=(complexf*)malloc(sizeof(complexf)*fft_size);
    FFT_PLAN_T* plan_taps = make_fft_c2c(fft_size, taps, taps_fft, 1, 0); //forward, don't benchmark (we need this only once)

    //make FFT plans for continously processing the input
    complexf* input = fft_malloc(fft_size*sizeof(complexf));
    complexf* input_fourier = fft_malloc(fft_size*sizeof(complexf));
    FFT_PLAN_T* plan_forward = make_fft_c2c(fft_size, input, input_fourier, 1, 1); //forward, do benchmark

    complexf* output_fourier = fft_malloc(fft_size*sizeof(complexf));
    complexf* output_1 = fft_malloc(fft_size*sizeof(complexf));
    complexf* output_2 = fft_malloc(fft_size*sizeof(complexf));
    //we create 2x output buffers so that one will preserve the previous overlap:
    FFT_PLAN_T* plan_inverse_1 = make_fft_c2c(fft_size, output_fourier, output_1, 0, 1); //inverse, do benchmark
    FFT_PLAN_T* plan_inverse_2 = make_fft_c2c(fft_size, output_fourier, output_2, 0, 1);







    //calculate the FFT size and the other length parameters
        int taps_length_BPF;
        int fft_size_BPF;
        int input_size;
        int overlap_length;
 



//float* a;
//float* b;
//float* c;

//FIRFilter    *iFilter;
//FIRFilter    *qFilter;
//Decimator    *d;


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

// --- shift_addition_cc

    bigbufs=1;
    starting_phase=0;
    decimation=1;
    starting_phase=0;


    LO=14100000;
    SetFrequency=14074000;
    SampleRate=1200000;

    rate=(LO-SetFrequency)/SampleRate;
    shift_addition_data_t d=decimating_shift_addition_init(rate, decimation);

    s.decimation_remain=0;
    s.starting_phase=0;


//--- parameters for fir_decimate_cc

    factor=25;
    transition_bw = 0.05;
    window = HAMMING;
    taps_length=firdes_filter_len(transition_bw);
    padded_taps_length = taps_length;

    taps=(float*)malloc(taps_length*sizeof(float));

    firdes_lowpass_f(taps,taps_length,0.5/(float)factor,window);

    input_skip=0;
    output_size=0;


    window_BPF=WINDOW_DEFAULT;
    low_cut=0.0;
    high_cut=0.5;
    transition_bw_BPF=0.05;

    //calculate the FFT size and the other length parameters
    int taps_length_BPF=firdes_filter_len(transition_bw); //the number of non-zero taps
    int fft_size_BPF=next_pow2(taps_length); //we will have to pad the taps with zeros until the next power of 2 for FFT

    //calculate the FFT size and the other length parameters
    taps_length_BPF=firdes_filter_len(transition_bw_BPF); //the number of non-zero taps
    fft_size_BPF=next_pow2(taps_length_BPF); //we will have to pad the taps with zeros until the next power of 2 for FFT

    //the number of padding zeros is the number of output samples we will be able to take away after every processing step, and it looks sane to check if it is large enough.
    if (fft_size_BPF-taps_length_BPF<200) fft_size_BPF<<=1;
 
    input_size = fft_size_BPF - taps_length_BPF + 1;
    overlap_length = taps_length_BPF - 1;


    //prepare making the filter and doing FFT on it
    taps_BPF=(complexf*)calloc(sizeof(complexf),fft_size); //initialize to zero
    taps_fft_BPF=(complexf*)malloc(sizeof(complexf)*fft_size);
    plan_taps_BPF = make_fft_c2c(fft_size, taps, taps_fft, 1, 0); //forward, don't benchmark (we need this only once)

    //make FFT plans for continously processing the input
    input_BPF = fft_malloc(fft_size*sizeof(complexf));
    input_fourier_BPF = fft_malloc(fft_size*sizeof(complexf));
    plan_forward_BPF = make_fft_c2c(fft_size, input_BPF, input_fourier_BPF, 1, 1); //forward, do benchmark

    output_fourier_BPF = fft_malloc(fft_size*sizeof(complexf));
    output_1_BPF = fft_malloc(fft_size*sizeof(complexf));
    output_2_BPF = fft_malloc(fft_size*sizeof(complexf));
    //we create 2x output buffers so that one will preserve the previous overlap:
    plan_inverse_1_BPF = make_fft_c2c(fft_size, output_fourier_BPF, output_1_BPF, 0, 1); //inverse, do benchmark
    plan_inverse_2_BPF = make_fft_c2c(fft_size, output_fourier_BPF, output_2_BPF, 0, 1);

    //we initialize this buffer to 0 as it will be taken as the overlap source for the first time:
    for(int i=0;i<fft_size;i++) iof(plan_inverse_2_BPF->output,i)=qof(plan_inverse_2_BPF->output,i)=0;
    for(int i=input_size_BPF;i<fft_size;i++) iof(input_BPF,i)=qof(input_BPF,i)=0; //we pre-pad the input buffer with zeros

//---

    sendbufsize(the_bufsize/decimation);   //Hay que alocar el buffer como la diferencia entre lo que entra y lo que diezmo
    malloc float* baseband_output
    malloc float* output_buffer
    malloc float* input_FIR
    malloc float* output_FIR
    
    fprintf(stderr,"%s::receive() Object creation completed\n",PROGRAMID);
  
}
//*--------------------------------------------------------------------------------------------------
// decimate: take one sample and compute one filterd output
//*--------------------------------------------------------------------------------------------------
int receive::decode(unsigned char* baseband_buffer,int baseband_len,short *audio_buffer) {

    //convert_u8_f(baseband_buffer, baseband_output, baseband_len);
    for(int i=0;i<baseband_len;i++) {
        baseband_output[i]=((float)baseband_buffer[i])/(UCHAR_MAX/2.0)-1.0; //@convert_u8_f
    }

    //shift_addition_cc { (LO-f)/SR }

    data=shift_addition_init(rate);
    remain=baseband_len;
    ibufptr=baseband_output;
    obufptr=output_buffer;

    while(remain) {
         current_size=(remain>baseband_len)?baseband_len:remain;    // OJO con esto ver con respecto a los buffers 
         starting_phase=shift_addition_cc((complexf*)ibufptr, (complexf*)obufptr, current_size, data, starting_phase);
         ibufptr+=current_size*2;
         obufptr+=current_size*2;
         remain-=current_size;
    }

    // fir_decimate_cc 25 0.05 HAMMING

    // ESTO HAY QUE REVISARLO MUY DETALLADAMENTE

    input_FIR=output_buffer;
    

    while (env_csdr_fixed_big_bufsize < taps_length*2) env_csdr_fixed_big_bufsize*=2; //temporary fix for buffer size if [transition_bw] is low

    sendbufsize(the_bufsize/factor);

    output_size=fir_decimate_cc((complexf*)input_FIR, (complexf*)output_FIR, the_bufsize, factor, taps, padded_taps_length);
    input_skip=factor*output_size;
    memmove((complexf*)input_buffer,((complexf*)input_buffer)+input_skip,(the_bufsize-input_skip)*sizeof(complexf)); //memmove lets the source and destination overlap


    // bandpass_fir_fft_cc 0 0.5 0.05



    if(!sendbufsize(getbufsize())) return -2;


    for(;;) {
            //make the filter
       errhead(); fprintf(stderr,"filter initialized, low_cut = %g, high_cut = %g\n",low_cut,high_cut);
       firdes_bandpass_c(taps, taps_length, low_cut, high_cut, window);
       fft_execute(plan_taps);

       for(int odd=0;;odd=!odd) { //the processing loop {
          FEOF_CHECK;
          fread(input, sizeof(complexf), input_size, stdin);
          FFT_PLAN_T* plan_inverse = (odd)?plan_inverse_2:plan_inverse_1;
          FFT_PLAN_T* plan_contains_last_overlap = (odd)?plan_inverse_1:plan_inverse_2; //the other
          complexf* last_overlap = (complexf*)plan_contains_last_overlap->output + input_size; //+ fft_size - overlap_length;
          apply_fir_fft_cc (plan_forward, plan_inverse, taps_fft, last_overlap, overlap_length);
          int returned=fwrite(plan_inverse->output, sizeof(complexf), input_size, stdout);
          if(read_fifo_ctl(fd,"%g %g\n",&low_cut,&high_cut)) break;
          TRY_YIELD;
        }
      }

}
