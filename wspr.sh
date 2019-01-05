#!/bin/sh

DPATH="/home/pi/WsprryPi"
CALLSIGN="LU7DID"
LOCATOR="GF05"
POWER="20"
BAND="20m 0 0 0 0 "
OPTS="-r -o -x 1 -s "
WSPR="wspr"

cd $DPATH
echo starting the daemon
while :
do
  echo starting a new WSPR beacon cycle
  vcgencmd measure_volts core
  vcgencmd measure_temp
  vcgencmd  get_throttled 
  $DPATH/$WSPR $OPTS $CALLSIGN $LOCATOR $POWER $BAND
  echo "waiting [1/4]"
  sleep 120
  echo "waiting [2/4]"
  sleep 120
  echo "waiting [3/4]"
  sleep 120
  echo "waiting [4/4]"
  sleep 60
  echo completing a WSPR beacon cycle
done
