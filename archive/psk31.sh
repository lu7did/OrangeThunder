#!/bin/sh
echo -n "CQ CQ CQ DE HA7ILM HA7ILM HA7ILM PSE K " | \
csdr psk31_varicode_encoder_u8_u8 | \
csdr differential_encoder_u8_u8 | \
csdr psk_modulator_u8_c 2 | \
csdr gain_ff 0.25 | \
csdr psk31_interpolate_sine_cc 256 | \
csdr shift_addition_cc 0.125 | \
csdr realpart_cf | \
csdr convert_f_s16 | \
mplayer -cache 1024 -quiet \
 -rawaudio samplesize=2:channels=1:rate=8000 -demuxer rawaudio - &

sleep 10
sudo killall mplayer
sleep 1
echo "termine la faena"

exit 0
