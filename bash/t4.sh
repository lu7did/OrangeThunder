#!/bin/sh

gpio mode "12" out
gpio -g write "12" 1

cat sampleaudio.wav  | csdr convert_i16_f | csdr fractional_decimator_ff 8.0 --prefilter \
  | csdr fir_interpolate_cc 2 |  csdr dsb_fc \
  | csdr bandpass_fir_fft_cc 0.002 0.24 0.05 | csdr fastagc_ff \
  | sudo sendiq -i /dev/stdin -s 6000 -f "$1" -t float

gpio -g write "12" 0

