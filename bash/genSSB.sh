#!/bin/sh


python turnon.py
#cat sampleaudio.wav  | /home/pi/OrangeThunder/bin/genSSB -f "$1" -s 6000 | sudo /home/pi/rpitx/sendiq -i /dev/stdin -s 6000 -f "$1" -t float
arecord -c1 -r48000 -D hw:Loopback,0 -fS16_LE -  | /home/pi/OrangeThunder/bin/genSSB -f "$1" -s 6000 | sudo /home/pi/rpitx/sendiq -i /dev/stdin -s 6000 -f "$1" -t float
python turnoff.py
