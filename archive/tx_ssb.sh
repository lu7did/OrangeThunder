#!/bin/sh
#*-----------------------------------------------------------------*
#* tx_ssb.sh
#* Experimental SSB generator chain for FT8 transceiver
#*
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
#* Usage:
#* ssb.sh FREQ [in Hz]
#*-----------------------------------------------------------------*
arecord -c1 -r48000 -D default -fS16_LE - | csdr convert_i16_f \
  | csdr fir_interpolate_cc 2 | csdr dsb_fc \
  | csdr bandpass_fir_fft_cc 0.002 0.06 0.01 | csdr fastagc_ff \
  | sudo ./sendiq -i /dev/stdin -s 96000 -f "$1" -t float

