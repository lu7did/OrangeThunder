#!/bin/sh


python turnon.py
cat sampleaudio.wav | /home/pi/OrangeThunder/bin/genSSB -t 2 -v 2 -d | sudo /home/pi/rpitx/sendiq -i /dev/stdin -s 6000 -f 14074000 -t float
python turnoff.py
