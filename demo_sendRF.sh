#!/bin/sh
#*-----------------------------------------------------------------------
#* demo_sendRF
#* Script to implement a simple USB transceiver using the OrangeThunder platformm
#* A remote pipe is implemented to carry CAT commands
#* Sound is feed thru the arecord command (PulseAudio ALSA), proper hardware
#* interface needs to be established. (-v parameter enables VOX)
#* the script needs to be called with the frequency in Hz as a parameter
#*            ./demo_genSSB.sh   (transmit at 14074000 Hz)
#*-----------------------------------------------------------------------
clear
echo "$0 (Orange Thunder simple USB transceiver)"

#*----- This is the configuration to operate PixiePi using a USB soundcard
#*----- genSSB operates with a 2 secs timeout auto PTT (-v), VOX enabled (-x) and DDS enabled (-d) 
arecord -c1 -r48000 -D hw:1 -fS16_LE - | genSSB -v 2 -x -d | sudo /home/pi/OrangeThunder/bin/sendRF -i /dev/stdin -s 6000 -f 7074000 -d 
