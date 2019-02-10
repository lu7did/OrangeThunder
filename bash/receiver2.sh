#!/bin/sh
#*-----------------------------------------------------------------*
#* receiver2.sh
#* Experimental SSB decoder chain for FT8 transceiver based on rtl_fm
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
#* receiver2.sh
#* Requires rtl-sdr package forked at lu7did/rtl-sdr which implements
#* a minor modification to allow the direct sampling mode on Q branch
#* for the RTL-SDR dongle
#*-----------------------------------------------------------------*
FREQ="14095600"
SAMPLERATE="1200000"
AUDIORATE="48000"
rtl_fm -M usb -g 45 -s $SAMPLERATE -f $FREQ -r 48k -l 0 -E direct \
    | mplayer -nocache -rawaudio samplesize=2:channels=1:rate=$AUDIORATE -demuxer rawaudio -

