#!/bin/sh
#*-----------------------------------------------------------------*
#* tx_PSK31
#* Experimental PSK31/SSB generator
#*
#* tx_PSK31.sh FREQ [in Hz] "MESSAGE"
#*-----------------------------------------------------------------*
#f="14097000"
echo "***********************************************"
echo "* PSK Generator                               "*
echo "***********************************************"
echo "Freq=$1 Message=$2"
f="$1"
echo "Starting SSB Generator at $f"
mkfifo pskfork || exit
/home/pi/whisper/ssb.sh $f 2> pskfork &
pid1=$!
while read line; do
  case $line in
    *high_cut*) echo "$line"; echo "found KEY=high_cut"; break;;
    *) echo "$line";;
  esac
done < pskfork

echo "Removing FIFO buffer"
rm -f pskfork  2>&1 > /dev/null

m="$2"
echo "Sending message frame($m)"

echo $(echo -n "$m") |
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

echo "Waiting for termination"
sleep 20

echo "cleaning up processes"
sudo killall mplayer 2>&1 > /dev/null
sudo kill $pid1  2>&1 > /dev/null
sudo killall csdr  2>&1 > /dev/null
#sudo killall arecord  2>&1 > /dev/null
#sudo killall sendiq  2>&1 > /dev/null

exit 0
