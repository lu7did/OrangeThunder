
#!/bin/sh

/home/pi/OrangeThunder/bin/rtl_fm -M usb -f 14.074M -s 4000 -E direct | mplayer -nocache -af volume=-10 -rawaudio samplesize=2:channels=1:rate=4000 -demuxer rawaudio -
