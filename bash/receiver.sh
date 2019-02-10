#!/bin/sh
#// receiver
#// Execute the rtl_sdr front end script, wait for proper start and then
#// launch the decoder backend into the virtual cable
#// 
#// Pre-requisite packages
#// ntp pulseaudio csdr pavucontrol nmap rtl-sdr mplayer 
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
#* Valid for WSPR (14095600) replace by 14074000 for FT8
#*----------------------------------------------------------------------------
LO="14100000"
SAMPLERATE="1200000"
FREQ="14095600"
FX=$(python -c "print float($LO-$FREQ)/$SAMPLERATE")
LOCALHOST="127.0.0.1"
AUDIORATE="48000"

#*----------------------------------------------------------------------------
# Clean up previous execution
#*----------------------------------------------------------------------------
sudo rm -r fiforx > /dev/null 2> /dev/null

sudo ps -aux | for p in `pgrep "rtl_sdr"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "csdr"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "ncat"`; do echo "killing "$p; sudo kill -9 $p; done
sudo ps -aux | for p in `pgrep "mplayer"`; do echo "killing "$p; sudo kill -9 $p; done

#*--- Launch the command, pipe StdErr of rtl_sdr into the fiforx
#*--- Exit if an error occurs

mkfifo fiforx || exit
exec rtl_sdr -s $SAMPLERATE -f $LO -D 2 - 2> fiforx | csdr convert_u8_f | ncat -4l 4952 -k --send-only --allow $LOCALHOST  &

#*--- while executing watch for the "async" word to appear, that means
#*--- the startup has been completed

while read line; do
    case $line in
        *async*) echo $line; echo "found KEY=async"; break;;
#       *Failed*) echo "Failed found"; exit;;
        *) echo "$line";;
    esac
done < fiforx

#*--- clean up the pipe, it is not longer needed

rm -f fiforx

#*---- Now launch the decoder backend

exec ncat -v $LOCALHOST 4952 | csdr shift_addition_cc $FX | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 | csdr realpart_cf | csdr agc_ff | csdr limit_ff | csdr convert_f_s16 | mplayer -nocache -rawaudio samplesize=2:channels=1:rate=$AUDIORATE -demuxer rawaudio - &

exit 0
