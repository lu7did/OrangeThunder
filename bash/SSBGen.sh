#!/bin/sh

#gpio mode "12" out
#gpio -g write "12" 1

python turnon.py
cat sampleaudio.wav  | sudo /home/pi/PixiePi/bin/SSBGen -i /dev/stdin -s $2 -f "$1" $3
python turnoff.py
#gpio -g write "12" 0

