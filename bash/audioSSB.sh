#!/bin/sh


python turnon.py
cat sampleaudio.wav | /home/pi/OrangeThunder/bin/genSSB -t 2 | sudo /home/pi/rpitx/sendiq -i /dev/stdin -s 6000 -f "$1" -t float
python turnoff.py
