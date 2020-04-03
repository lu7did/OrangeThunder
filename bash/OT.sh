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

sudo /home/pi/OrangeThunder/bin/OT | mplayer -nocache -af volume=-10 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
#sudo /home/pi/OrangeThunder/bin/OT
exit 0
