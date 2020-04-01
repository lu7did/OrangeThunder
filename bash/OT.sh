#!/bin/sh
#// receiver
#// Execute the rtl_sdr front end script, wait for proper start and then
#// launch the decoder backend into the ASA
#//
#// See accompanying README
#// file for a description on how to use this code.
#// based on the package WsprryPi (https://github.com/JamesP6000/WsprryPi)
#// License:
#//   This program is free software: you can redistribute it and/or modify
#//   it under the terms of the GNU General Public License as published by
#//   the Free Software Foundation, either version 2 of the License, or
#//   (at your option) any later version.
#//
#//   This program is distributed in the hope that it will be useful,
#//   but WITHOUT ANY WARRANTY; without even the implied warranty of
#//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//   GNU General Public License for more details.
#//
#//   You should have received a copy of the GNU General Public License
#//   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#// lu7did: initial load
#*----------------------------------------------------------------------------
#* Initialization
#* DO NOT RUN EITHER AS A rc.local script nor as a systemd controlled service
#*----------------------------------------------------------------------------
FO="14074000"
SAMPLERATE="1200000"
LO="14100000"
MODE="FT8"

#*----------------------------------------------------------------------------
# Clean up previous execution
#*----------------------------------------------------------------------------
echo "cleaning up previous pipes and processes"
sudo rm -r fiforx > /dev/null 2> /dev/null
sudo rm -r rtlsdr > /dev/null 2> /dev/null

sudo ps -aux | for p in `pgrep "rtl_sdr"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "csdr"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "ncat"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "mplayer"`; do echo "killing "$p; sudo kill -9 $p; done

echo "launching new FIFO pipes"
#*--- Launch the command, pipe StdErr of rtl_sdr into the fiforx
mkfifo fiforx || exit
#mkfifo rtlsdr || exit
#exec rtl_sdr -s $SAMPLERATE -f $LO -D 2 - 2> fiforx | csdr convert_u8_f | ncat -4l 4952 -k --send-only --allow 127.0.0.1  &

echo "executing rtl_sdr"
exec rtl_sdr -s $SAMPLERATE -f $LO -D 2 - 2> fiforx | csdr convert_u8_f | ncat -4l 4952 -k --send-only --allow 127.0.0.1  &
#exec rtl_sdr -s $SAMPLERATE -f $LO -D 2 - 2> fiforx | csdr convert_u8_f

#*--- while executing watch for the "async" word to appear, that means
#*--- startup has been completed

echo "waiting for keyword either failure or success" 
while read line; do
    echo $line
    case $line in
        *async*) echo $line; echo "found KEY=async"; break;;
        *Failed*) echo "Failed found"; exit;;
        *) echo "$line";;
    esac
done < fiforx

#*--- clean up the pipe, is not longer needed

#echo "cleaning up service pipe"
rm -f fiforx

FX=$(python -c "print float($LO-$FO)/$SAMPLERATE")
echo "Arg $1 MODE=$MODE FO=$FO LO=$LO SAMPLERATE=$SAMPLERATE FX=$FX"

exec ncat -v 127.0.0.1 4952 | csdr shift_addition_cc $FX | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 | csdr realpart_cf | csdr agc_ff | csdr limit_ff | csdr convert_f_s16 | mplayer -nocache -rawaudio samplesize=2:channels=1:rate=48000 -demuxer rawaudio - &
#exec cat rtlsdr | csdr convert_u8_f | csdr shift_addition_cc $FX | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 | csdr realpart_cf | csdr agc_ff | csdr limit_ff | csdr convert_f_s16
#cat rtlsdr

exit 0
