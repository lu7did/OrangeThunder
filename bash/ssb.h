#define  BUFFER_SIZE  4096
#define  SETBUF_DEFAULT_BUFSIZE 1024
#define  FREAD_U8    fread (input_buffer,    sizeof(unsigned char), the_bufsize, stdin)
#define  FWRITE_U8   fwrite (output_buffer,  sizeof(unsigned char), the_bufsize, stdout)
#define  FREAD_R     fread (input_buffer,    sizeof(float),      the_bufsize, stdin)
#define  FREAD_C     fread (input_buffer,    sizeof(float)*2,    the_bufsize, stdin)
#define  FWRITE_R    fwrite (output_buffer,  sizeof(float),      the_bufsize, stdout)
#define  FWRITE_C    fwrite (output_buffer,  sizeof(float)*2,    the_bufsize, stdout)
#define  FEOF_CHECK  if(feof(stdin)) return 0
typedef enum window_s
{
    WINDOW_BOXCAR, WINDOW_BLACKMAN, WINDOW_HAMMING
} window_t;

#define WINDOW_DEFAULT WINDOW_HAMMING



float*   input_buffer;
unsigned char* buffer_u8;
float    *output_buffer;
float    *dsb_buffer;
short    *buffer_i16;
float    *temp_f;
int      the_bufsize = SETBUF_DEFAULT_BUFSIZE;;

float rate =8.0;
int num_poly_points = 12;
int use_prefilter = 1;
float transition_bw=0.03;
window_t window = WINDOW_DEFAULT;
//Generate filter taps
int taps_length = 0;
float* taps = NULL;
fractional_decimator_ff_t d; 
int taps_length;
float *taps;
int input_skip=0;
int output_size=0;
float* interp_output_buffer = 0.0;



float q_value = 0;


        float low_cut=0.002;
        float high_cut=0.24;
        float transition_bw=0.05;
        window_t window = WINDOW_DEFAULT;
        int fd;

int factor=2;

int taps_length;
int fft_size;
int intput_size;
int overlap_length;

int taps_length;


        complexf* taps;
        complexf* taps_fft;
        FFT_PLAN_T* plan_taps;

        //make FFT plans for continously processing the input
        complexf* input;
        complexf* input_fourier;
        FFT_PLAN_T* plan_forward;

        complexf* output_fourier;
        complexf* output_1;
        complexf* output_2;
        //we create 2x output buffers so that one will preserve the previous overlap:
        FFT_PLAN_T* plan_inverse_1;
        FFT_PLAN_T* plan_inverse_2;


//*-------------------------------------------------------------------------------------------------------------------------------------------

int initialize_buffers()
{
    fprintf(stderr,"SSB() buffer size set to %d\n", the_bufsize); }
    input_buffer  =  (float*)        malloc(the_bufsize*sizeof(float) * 2); //need the 2× because we might also put complex floats into it
    output_buffer = (float*)        malloc(the_bufsize*sizeof(float) * 2);
    dsb_buffer    = (float*)        malloc(the_bufsize*sizeof(float) * 2);
    buffer_u8     =     (unsigned char*)malloc(the_bufsize*sizeof(unsigned char));
    buffer_i16    =    (short*)        malloc(the_bufsize*sizeof(short));
    temp_f        =        (float*)        malloc(the_bufsize*sizeof(float) * 4);
    return the_bufsize;
}


void setupSSB() {

//* Setup for fractional_decimator_ff ()

    taps_length = firdes_filter_len(transition_bw);
    fprintf(stderr,"taps_length = %d\n",taps_length);
    taps = (float*)malloc(sizeof(float)*taps_length);
    firdes_lowpass_f(taps, taps_length, 0.5/(rate-transition_bw), window); //0.6 const to compensate rolloff
    (fractional_decimator_ff_t) d = fractional_decimator_ff_init(rate, num_poly_points, taps, taps_length); 



        bigbufs=1;
        factor=2;
        //float transition_bw = 0.05;
        taps_length=firdes_filter_len(transition_bw);
        while (env_csdr_fixed_big_bufsize < taps_length*2) env_csdr_fixed_big_bufsize*=2; //temporary fix for buffer size if [transition_bw] is low
        //fprintf(stderr, "env_csdr_fixed_big_bufsize = %d\n", env_csdr_fixed_big_bufsize);

        taps=(float*)malloc(taps_length*sizeof(float));

        firdes_lowpass_f(taps,taps_length,0.5/(float)factor,window);
        interp_output_buffer = (float*)malloc(sizeof(float)*2*the_bufsize*factor);


        //calculate the FFT size and the other length parameters
        taps_length=firdes_filter_len(transition_bw); //the number of non-zero taps
        fft_size=next_pow2(taps_length); //we will have to pad the taps with zeros until the next power of 2 for FFT
        //the number of padding zeros is the number of output samples we will be able to take away after every processing step, and it looks sane to check if it is large enough.
        if (fft_size-taps_length<200) fft_size<<=1;
        input_size = fft_size - taps_length + 1;
        overlap_length = taps_length - 1;
        fprintf(stderr,"(fft_size = %d) = (taps_length = %d) + (input_size = %d) - 1\n(overlap_length = %d) = taps_length - 1\n", fft_size, taps_length, input_size, overlap_length );
        //if (fft_size<=2) return badsyntax("FFT size error.");



    return;
}

//*-------------------------------------------------------------------------------------------------------------------------------------------
//* convert_i16_f
//* convert from integer 16 bits to float
//*-------------------------------------------------------------------------------------------------------------------------------------------


        //for(;;)
        //{
        FEOF_CHECK;
        fread(buffer_i16, sizeof(short), the_bufsize, stdin);
        convert_i16_f(buffer_i16, output_buffer, the_bufsize);
        //    FWRITE_R;
        //    TRY_YIELD;
        //}

//*-------------------------------------------------------------------------------------------------------------------------------------------
//* fractional_decimator_ff 8.0 --prefilter
//* decimate input by a factor of 8 (input is float)
//*-------------------------------------------------------------------------------------------------------------------------------------------

        //for(;;)
        //{
        //    FEOF_CHECK;
        //    if(d.input_processed==0) d.input_processed=the_bufsize;
        //    else memcpy(input_buffer, input_buffer+d.input_processed, sizeof(float)*(the_bufsize-d.input_processed));
        //    fread(input_buffer+(the_bufsize-d.input_processed), sizeof(float), d.input_processed, stdin);
        fractional_decimator_ff(input_buffer, output_buffer, the_bufsize, &d);
        //    fwrite(output_buffer, sizeof(float), d.output_size, stdout);
        //    fprintf(stderr, "os = %d, ip = %d\n", d.output_size, d.input_processed);
        //    TRY_YIELD;
        //}


//*-------------------------------------------------------------------------------------------------------------------------------------------
//* fir_interpolate_cc 2
//* Convert from single float to double float (complex)
//*-------------------------------------------------------------------------------------------------------------------------------------------


        //for(;;)
        //{

          //FEOF_CHECK;
          output_size=fir_interpolate_cc((complexf*)output_buffer, (complexf*)interp_output_buffer, the_bufsize, factor, taps, taps_length);
          //fprintf(stderr, "os %d\n",output_size);
          //fwrite(interp_output_buffer, sizeof(complexf), output_size, stdout);
          //TRY_YIELD;

          input_skip=output_size/factor;
          memmove((complexf*)output_buffer,((complexf*)output_buffer)+input_skip,(the_bufsize-input_skip)*sizeof(complexf)); //memmove lets the source and destination overlap

          //fread(((complexf*)input_buffer)+(the_bufsize-input_skip), sizeof(complexf), input_skip, stdin);
          //fprintf(stderr,"iskip=%d output_size=%d start=%x target=%x skipcount=%x \n",input_skip,output_size,input_buffer, ((complexf*)input_buffer)+(BIG_BUFSIZE-input_skip),(BIG_BUFSIZE-input_skip));

        //}

//*----------------------------------------------------------------------------------------------------------------------------------
//* dsb_fc
//* populate the Q signal component
//*----------------------------------------------------------------------------------------------------------------------------------
        //for(;;)
        //{

        //    FEOF_CHECK;
        //    FREAD_R;
        //    for(int i=0;i<the_bufsize;i++)
        //    {
                iof(dsb_buffer,i)=output_buffer[i];
                qof(dsb_buffer,i)=q_value;
        //    }
        //    FWRITE_C;
        //    TRY_YIELD;

        //}
//*-------------------------------------------------------------------------------------------------------------------------------------------
//* bandpass_fir_fft_cc 0.002 0.24 0.05
//* limit image
//*-------------------------------------------------------------------------------------------------------------------------------------------
//        if(fd=init_fifo(argc,argv))
//        {
//            while(!read_fifo_ctl(fd,"%g %g\n",&low_cut,&high_cut)) usleep(10000);
//            if(argc<=4) return badsyntax("need more required parameters (transition_bw)");
//        }
//        else
//        {
//            if(argc<=4) return badsyntax("need required parameters (low_cut, high_cut, transition_bw)");
//            sscanf(argv[2],"%g",&low_cut);
//            sscanf(argv[3],"%g",&high_cut);
//        }
//        sscanf(argv[4],"%g",&transition_bw);
//        if(argc>=6) window=firdes_get_window_from_string(argv[5]);
//        else { errhead(); fprintf(stderr,"window = %s\n",firdes_get_string_from_window(window)); }



//        if(!sendbufsize(getbufsize())) return -2;

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
        //we initialize this buffer to 0 as it will be taken as the overlap source for the first time:
        for(int i=0;i<fft_size;i++) iof(plan_inverse_2->output,i)=qof(plan_inverse_2->output,i)=0;

        for(int i=input_size;i<fft_size;i++) iof(input,i)=qof(input,i)=0; //we pre-pad the input buffer with zeros

        for(;;)
        {
            //make the filter
            errhead(); fprintf(stderr,"filter initialized, low_cut = %g, high_cut = %g\n",low_cut,high_cut);
            firdes_bandpass_c(taps, taps_length, low_cut, high_cut, window);
            fft_execute(plan_taps);

            for(int odd=0;;odd=!odd) //the processing loop
            {
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

//*-------------------------------------------------------------------------------------------------------------------------------------------
//* fastagc_ff
//* process AGC on signal
//*-------------------------------------------------------------------------------------------------------------------------------------------

        static fastagc_ff_t input; //is in .bss and gets cleared to zero before main()

        input.input_size=1024;
        if(argc>=3) sscanf(argv[2],"%d",&input.input_size);

        getbufsize(); //dummy
        sendbufsize(input.input_size);

        input.reference=1.0;
        if(argc>=4) sscanf(argv[3],"%g",&input.reference);

        //input.max_peak_ratio=12.0;
        //if(argc>=5) sscanf(argv[3],"%g",&input.max_peak_ratio);

        input.buffer_1=(float*)calloc(input.input_size,sizeof(float));
        input.buffer_2=(float*)calloc(input.input_size,sizeof(float));
        input.buffer_input=(float*)malloc(sizeof(float)*input.input_size);
        float* agc_output_buffer=(float*)malloc(sizeof(float)*input.input_size);
        for(;;)
        {
            FEOF_CHECK;
            fread(input.buffer_input, sizeof(float), input.input_size, stdin);
            fastagc_ff(&input, agc_output_buffer);
            fwrite(agc_output_buffer, sizeof(float), input.input_size, stdout);
            TRY_YIELD;
        }



        return;
}
