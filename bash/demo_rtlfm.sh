#!/bin/sh
#*-----------------------------------------------------------------------
#* demo_rtlfm
#* Script to implement a SSB receiver using the OrangeThunder platformm
#* Sound is feed thru the aplay command (PulseAudio ALSA), proper hardware
#*            ./demo_rtlfm.sh   (receives at 14074000 Hz)
#*-----------------------------------------------------------------------
clear
#*----------------------------------------*
#* Launching socat server                 *
#*----------------------------------------*
sudo pkill -9 -f rtl_fm
sudo pkill -9 -f mplayer

/home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14.074M -s 4000 -E direct | mplayer -nocache -af volume=0 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
