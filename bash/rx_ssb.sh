#!/bin/sh
#*-----------------------------------------------------------------*
#* rx_ssb.sh
#* Experimental SSB decoder chain for FT8 transceiver
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
#* usage:
#* rx_ssb.sh FREQ [in Hz]
#*-----------------------------------------------------------------*
QRG="14074000"
SAMPLERATE="1200000"
rtl_sdr -s 1200000 -f 14100000 -D 2 - | csdr convert_u8_f \
        | csdr shift_addition_cc `python -c "print float(14100000-14074000)/1200000"` \
        | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 \
        | csdr realpart_cf | csdr agc_ff | csdr limit_ff  \
        | csdr convert_f_s16 \
        | aplay -t raw -f S16_LE -c1 -r48000

