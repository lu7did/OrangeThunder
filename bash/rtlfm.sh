
#!/bin/sh

#rtl_fm -M usb -f 14.074M -s 48000 -E direct | aplay -r 48000 -f S16_LE -t raw -c 1
#rtl_fm -M usb -f 14.074M -s 4000 -E direct | mplayer -nocache -af volume=10 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
/home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14.074M -s 4000 -E direct | mplayer -nocache -af volume=-10 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
