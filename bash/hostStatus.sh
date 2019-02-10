#!/bin/sh
#*------------------------------------------------------------------------------------------
#* hostStatus.sh
#* Collect host status information
#*------------------------------------------------------------------------------------------
#// See accompanying README file for a description on how to use this code.
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
#//*========================================================================
#// lu7did: initial load
#*----------------------------------------------------------------------------
#* Initialization
#* DO NOT RUN EITHER AS A rc.local script nor as a systemd controlled service
#*----------------------------------------------------------------------------

getCPU () {
sar 1 3 | grep "Media:" | while read a ; do
 echo $a | awk '{print $3 + $4 + $5 + $6 + $7}';
done
}

getTemp () {

TEMP=$(vcgencmd measure_temp)
echo $TEMP | cut -f2 -d"=" | cut -f1 -d"'"

}

getVolt () {

VOLT=$(vcgencmd measure_volts | cut -f2 -d"=" | cut -f1 -d"V" )
VOLT=$(python -c "print ('%.2f' % ($VOLT*1.0))" )
echo $VOLT 

}

getClock () {

CLOCK=$(vcgencmd measure_clock arm | cut -f2 -d"=")
FX=$(python -c "print float($CLOCK)/1000000")
echo $FX

}

getStatus () {

STATUS=$(vcgencmd get_throttled)
echo $STATUS | cut -f2 -d"="

}

getDASD () {
sudo df -k | grep "/dev/root" | awk '{ print $5 ; }' | cut -f1 -d"%"
}

STATE="T($(getTemp)°C) V($(getVolt)V) Clk($(getClock)MHz) St($(getStatus)) CPU($(getCPU)%) DASD($(getDASD)%)" 
echo $STATE | logger -i -t "TLM"
exit 0
