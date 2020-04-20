#!/bin/sh

python turnon.py
sudo /home/pi/OrangeThunder/bin/demo_genDDS -f 14074600 
python turnoff.py
