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
sudo pkill -9 -f aplay

/home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14074000 -s 4000 -E direct 2>/dev/null | mplayer -nocache -af volume=0 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
#/home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14074000 -s 4000 -E direct 2>/dev/null | mplayer -nocache -af volume=0 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
#rtl_fm -M usb -f 14074000 -s 44100  -E direct  | aplay -D hw:Loopback,1 -r 44100  -fS16_LE -
