#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "/home/pi/ft8_lib/ft8/unpack.h"
#include "/home/pi/ft8_lib/ft8/ldpc.h"
#include "/home/pi/ft8_lib/ft8/decode.h"
#include "/home/pi/ft8_lib/ft8/constants.h"
#include "/home/pi/ft8_lib/ft8/encode.h"

#include "/home/pi/ft8_lib/common/wave.h"
#include "/home/pi/ft8_lib/common/debug.h"
#include "/home/pi/ft8_lib/fft/kiss_fftr.h"

const int kMin_score = 40;		// Minimum sync score threshold for candidates
const int kMax_candidates = 120;
const int kLDPC_iterations = 25;
const int kMax_decoded_messages = 50;
const int kMax_message_length = 25;
const int kFreq_osr = 2;
const int kTime_osr = 2;
const float kFSK_dev = 6.25f;    // tone deviation in Hz and symbol rate
float signalDSP[256000];        //only 128000 used

//----
float hann_i(int i, int N) {

    if((N-1)==0) {
      fprintf(stderr,"%s:hann_i **ERROR** N-1 is zero, abandon\n",PROGRAMID);
      return 1;
    }
    float x = sinf((float)M_PI * i / (N - 1));
    return x*x;
}

//---

float hamming_i(int i, int N) {
    const float a0 = (float)25 / 46;
    const float a1 = 1 - a0;
    if((N-1)==0) {
      fprintf(stderr,"%s:hamming_i **ERROR** N-1 is zero, abandon\n",PROGRAMID);
      return 1;
    }
    float x1 = cosf(2 * (float)M_PI * i / (N - 1));
    return a0 - a1*x1;
}

//---

float blackman_i(int i, int N) {
    const float alpha = 0.16f; // or 2860/18608
    const float a0 = (1 - alpha) / 2;
    const float a1 = 1.0f / 2;
    const float a2 = alpha / 2;

    float x1 = cosf(2 * (float)M_PI * i / (N - 1));
    float x2 = 2*x1*x1 - 1; // Use double angle formula

    return a0 - a1*x1 + a2*x2;
}

//---

static float max2(float a, float b) {
    return (a >= b) ? a : b;
}

//---
// Compute FFT magnitudes (log power) for each timeslot in the signal
void extract_power(const float signalDSP[], ft8::MagArray * power) {

const int block_size = 2 * power->num_bins; // Average over 2 bins per FSK tone

            if (power->time_osr == 0) {
               fprintf(stderr,"%s:extract_power **ERROR** power->time_osr is zero, abandon\n",PROGRAMID);
               return;
            }
const int   subblock_size = block_size / power->time_osr;
const int   nfft = block_size * power->freq_osr; // We take FFT of two blocks, advancing by one
          
            if (nfft==0) {
               fprintf(stderr,"%s:extract_power() **ERROR** nfft is zero, abandon\n",PROGRAMID);
               return;
            }
const float fft_norm = 2.0f / nfft;

    float   window[nfft+10];
            for (int i = 0; i < nfft; ++i) {
               window[i] = hann_i(i, nfft);
            }

    size_t  fft_work_size;
            kiss_fftr_alloc(nfft, 0, 0, &fft_work_size);
            (TRACE>=0x02 ? fprintf(stderr,"%s:extract_power() Block size=%d Subblock size=%d nfft=%d FFT work area=%lu\n",PROGRAMID,block_size,subblock_size,nfft,fft_work_size) : _NOP);
    //LOG(LOG_INFO, "Block size = %d\n", block_size);
    //LOG(LOG_INFO, "Subblock size = %d\n", subblock_size);
    //LOG(LOG_INFO, "N_FFT = %d\n", nfft);
    //LOG(LOG_INFO, "FFT work area = %lu\n", fft_work_size);

        void *fft_work = malloc(fft_work_size*2);
             kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(nfft, 0, fft_work, &fft_work_size);

         int offset = 0;
       float max_mag = -100.0f;
        (TRACE>=0x02 ? fprintf(stderr,"%s:extract_power() Loop over possible offset\n",PROGRAMID) : _NOP);

             for (int i = 0; i < power->num_blocks; ++i) {
        // Loop over two possible time offsets (0 and block_size/2)
               for (int time_sub = 0; time_sub < power->time_osr; ++time_sub) {
                 kiss_fft_scalar timedata[nfft*2];
                 kiss_fft_cpx    freqdata[(nfft/2 + 1)*2];
          float  mag_db[(nfft/2 + 1)*2];

            // Extract windowed signal block
                 for (int j = 0; j < nfft; ++j) {
                    timedata[j] = window[j] * signalDSP[(i * block_size) + (j + time_sub * subblock_size)];
                 }
                 kiss_fftr(fft_cfg, timedata, freqdata);

            // Compute log magnitude in decibels
          
                for (int j = 0; j < nfft/2 + 1; ++j) {
             float mag2 = (freqdata[j].i * freqdata[j].i + freqdata[j].r * freqdata[j].r);
                   mag_db[j] = 10.0f * log10f(1E-10f + mag2 * fft_norm * fft_norm);
                }

            // Loop over two possible frequency bin offsets (for averaging)
                //float avg=0.0;
                //int   num=0;
                for (int freq_sub = 0; freq_sub < power->freq_osr; ++freq_sub) {                
                   for (int j = 0; j < power->num_bins; ++j) {
                       float db1 = mag_db[j * power->freq_osr + freq_sub];
                     //float db2 = mag_db[j * 2 + freq_sub + 1];
                     //float db = (db1 + db2) / 2;
                       float db = db1;
                       //avg=db+avg;
                       //num++;
                     //float db = sqrtf(db1 * db2);

                    // Scale decibels to unsigned 8-bit range and clamp the value
                   int scaled = (int)(2 * (db + 120));
                       power->mag[offset] = (scaled < 0) ? 0 : ((scaled > 255) ? 255 : scaled);
                      (TRACE>=0x03 ? fprintf(stderr,"%s:extractPower() Offset(%d) power(%d)\n",PROGRAMID,offset,power->mag[offset]) : _NOP);
                      ++offset;

                      if (db > max_mag) max_mag = db;
                   }
                }
                //if (num!=0) {
                //   (TRACE>=0x02 ? fprintf(stderr,"%s:extractPower() sum signal(%f) num signal(%d) average(%f)\n",PROGRAMID,avg,num,(float)avg/num) : _NOP);
                //}

             }
          }

    //LOG(LOG_INFO, "Max magnitude: %.1f dB\n", max_mag);
   (TRACE>=0x02 ? fprintf(stderr,"%s:extract power Max magnitude: %.1f dB\n",PROGRAMID,max_mag) : _NOP);
   free(fft_work);
}


void normalize_signal(float *signalDSP, int num_samples) {
   (TRACE>=0x02 ? fprintf(stderr,"%s:normalize_signal() samples(%d)\n",PROGRAMID,num_samples) : _NOP);

    float max_amp = 1E-5f;
    for (int i = 0; i < num_samples; ++i) {
        float amp = fabsf(signalDSP[i]);
        if (amp > max_amp) {
            max_amp = amp;
        }
    }
    for (int i = 0; i < num_samples; ++i) {
        if (max_amp!=0){ 
           signalDSP[i] /= max_amp;
        } else {
           signalDSP[i]=0;
           fprintf(stderr,"%s:normalize_signal() *** ERROR *** max_amp is zero\n",PROGRAMID);
        }
    }    
}


void print_tones(const uint8_t *code_map, const float *log174) {
    for (int k = 0; k < ft8::N; k += 3) {
        uint8_t max = 0;
        if (log174[k + 0] > 0) max |= 4;
        if (log174[k + 1] > 0) max |= 2;
        if (log174[k + 2] > 0) max |= 1;
        //LOG(LOG_DEBUG, "%d", code_map[max]);
    }
    //LOG(LOG_DEBUG, "\n");
   (TRACE>=0x02 ? fprintf(stderr,"%s:print_tones()\n",PROGRAMID) : _NOP);

}

//*===========================================================================================================================
//* FT8_process
//* Main decoding algorithm
//* Returns number of messages 
//*===========================================================================================================================
int FT8_process(int num_samples,int sample_rate,FT8msg* slotmsg) {

int  returnedMessages=0;
char buffer[128];
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process samples(%d) sample_rate(%d)\n",PROGRAMID,num_samples,sample_rate) : _NOP);
    normalize_signal(signalDSP, num_samples);

    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process returned from normalize_signal\n",PROGRAMID) : _NOP);
    // Compute DSP parameters that depend on the sample rate
    const int num_bins = (int)(sample_rate / (2 * kFSK_dev));
    const int block_size = 2 * num_bins;
    const int subblock_size = block_size / kTime_osr;
    const int nfft = block_size * kFreq_osr;
    const int num_blocks = (num_samples - nfft + subblock_size) / block_size;

    //LOG(LOG_INFO, "Sample rate %d Hz, %d blocks, %d bins\n", sample_rate, num_blocks, num_bins);
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process sample rate %d Hz, %d blocks, %d bins\n",PROGRAMID,sample_rate,num_blocks,num_bins) : _NOP);

    // Compute FFT over the whole signal and store it
    uint8_t mag_power[(num_blocks * kFreq_osr * kTime_osr * num_bins)+10];
    (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process sample mag_power defined\n",PROGRAMID) : _NOP);

    ft8::MagArray power = { 
        .num_blocks = num_blocks, 
        .num_bins = num_bins,
        .time_osr = kTime_osr,
        .freq_osr = kFreq_osr,
        .mag = mag_power
    };
    extract_power(signalDSP, &power);
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process extract_power processed\n",PROGRAMID) : _NOP);

    // Find top candidates by Costas sync score and localize them in time and frequency
    ft8::Candidate candidate_list[kMax_candidates+2];
    int num_candidates = ft8::find_sync(&power, ft8::kCostas_map, kMax_candidates, candidate_list, kMin_score);
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process find top candidates candidates(%d)\n",PROGRAMID,num_candidates) : _NOP);

    // TODO: sort the candidates by strongest sync first?

    // Go over candidates and attempt to decode messages
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process decoded message(%d) max message length(%d)\n",PROGRAMID,kMax_decoded_messages,kMax_message_length) : _NOP);
  
    char    decoded[kMax_decoded_messages+1][kMax_message_length+1];
    int     num_decoded = 0;
    (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process allocate memory for decoded candidates\n",PROGRAMID) : _NOP);


    for (int idx = 0; idx < num_candidates; ++idx) {
        ft8::Candidate &cand = candidate_list[idx];

        if (cand.score < kMin_score) continue;
        (TRACE>=0x03 ? fprintf(stderr,"%s:FT8_process processing candidate(%d) score(%d) time_o(%d) freq_o(%d) time_sub(%d) freq_sub(%d)\n",PROGRAMID,idx,cand.score,cand.time_offset,cand.freq_offset,cand.time_sub,cand.freq_sub) : _NOP);
        float freq_hz = 0.0;
        if (kFreq_osr != 0) {
          freq_hz  = (cand.freq_offset + (float)cand.freq_sub / kFreq_osr) * kFSK_dev;
        } else {
          fprintf(stderr,"%s:FT8_process *** ERROR *** kFreq_osr=0, abandoned\n",PROGRAMID);
          continue;
        }

        float time_sec=0;
        if (kFSK_dev!=0) {
            time_sec = (cand.time_offset + (float)cand.time_sub / kTime_osr) / kFSK_dev;
        } else {
            time_sec = 0;
            fprintf(stderr,"%s:FT8_process() time_sec is zero, abandon\n",PROGRAMID);
            continue;
        }

        (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process computing log174 with N+1(%d)\n",PROGRAMID,ft8::N+1) : _NOP);
        float   log174[ft8::N+1];
        ft8::extract_likelihood(&power, cand, ft8::kGray_map, log174);

        // bp_decode() produces better decodes, uses way less memory
        uint8_t plain[ft8::N+1];
        int     n_errors = 0;
        ft8::bp_decode(log174, kLDPC_iterations, plain, &n_errors);
        //ft8::ldpc_decode(log174, kLDPC_iterations, plain, &n_errors);

        if (n_errors > 0) {
            //LOG(LOG_DEBUG, "ldpc_decode() = %d (%.0f Hz)\n", n_errors, freq_hz);
            (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process ldpc_decode() = %d (%.0f Hz)\n",PROGRAMID,n_errors,freq_hz) : _NOP);
            continue;
        }

        (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process about to process sum_plain calculus\n",PROGRAMID) : _NOP);
        
        int sum_plain = 0;
        for (int i = 0; i < ft8::N; ++i) {
            sum_plain += plain[i];
        }

        if (sum_plain == 0) {
            // All zeroes message
            (TRACE>=0x02 ? fprintf(stderr,"%s:FT8_process all zeroes messages\n",PROGRAMID) : _NOP);
            continue;
        }
        (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process about to extract payload\n",PROGRAMID) : _NOP);
        
        // Extract payload + CRC (first ft8::K bits)
        uint8_t a91[ft8::K_BYTES];
        ft8::pack_bits(plain, ft8::K, a91);

        // Extract CRC and check it
        uint16_t chksum = ((a91[9] & 0x07) << 11) | (a91[10] << 3) | (a91[11] >> 5);
        a91[9] &= 0xF8;
        a91[10] = 0;
        a91[11] = 0;
        uint16_t chksum2 = ft8::crc(a91, 96 - 14);
        if (chksum != chksum2) {
            //LOG(LOG_DEBUG, "Checksum: message = %04x, CRC = %04x\n", chksum, chksum2);
           (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process Checksum: message = %04x, CRC = %04x\n",PROGRAMID,chksum,chksum2) : _NOP);
            continue;
        }

        char message[kMax_message_length+1];
        if (ft8::unpack77(a91, message) < 0) {
            continue;
        }

        (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process about to check for duplicates\n",PROGRAMID) : _NOP);

        // Check for duplicate messages (TODO: use hashing)
        bool found = false;
        for (int i = 0; i < num_decoded; ++i) {
            if (0 == strcmp(decoded[i], message)) {
                found = true;
                break;
            }
        }
        (TRACE >=0x03 ? fprintf(stderr,"%s:FT8_process checked for duplicates decoded(%d) N(%d)\n",PROGRAMID,num_decoded,ft8::N) : _NOP);

        if (!found && num_decoded < kMax_decoded_messages) {
            strcpy(decoded[num_decoded], message);
            ++num_decoded;

            // Fake WSJT-X-like output for now
            int snr = -10;    // TODO: compute SNR *** FAKE *** need to compute a good number
            (TRACE>=0x02 ? fprintf(stderr,"(%3d %4.1f %4d ~  %s)\n", cand.score, time_sec, (int)(freq_hz + 0.5f), message) : _NOP);

            slotmsg[returnedMessages].db=snr;
            slotmsg[returnedMessages].DT=time_sec;
            slotmsg[returnedMessages].offset=(int)(freq_hz+0.5f);
            slotmsg[returnedMessages].msg = (char*) malloc(strlen(message)+16);
            strcpy(slotmsg[returnedMessages++].msg,message);
         }
    }
    //LOG(LOG_INFO, "Decoded %d messages\n", num_decoded);
    (TRACE >=0x02 ? fprintf(stderr,"%s:FT8_process decoded %d messages\n",PROGRAMID,num_decoded) : _NOP);
    return num_decoded;
}
