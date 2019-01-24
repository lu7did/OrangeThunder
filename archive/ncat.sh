#!/bin/sh
ncat -v 127.0.0.1 4952 | csdr shift_addition_cc `python -c "print float(14100000-14095600)/1200000"` | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 | csdr realpart_cf | csdr agc_ff | csdr limit_ff | csdr convert_f_s16 | mplayer -nocache -rawaudio samplesize=2:channels=1:rate=48000 -demuxer rawaudio -

