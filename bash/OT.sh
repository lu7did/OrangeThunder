#!/bin/sh
#*-----------------------------------------------------------------------
#* OT.sh
#* Script to implement a SSB transceiver
#* A remote pipe is implemented to carry CAT commands
#* Sound is feed thru the arecord command (PulseAudio ALSA), proper hardware
#* interface needs to be established. (-a parameter enables VOX)
#* the script needs to be called with the frequency in Hz as a parameter
#*            ./Pi4D.sh [frequency in Hz]
#*-----------------------------------------------------------------------
echo "Orange Thunder based SSB transceiver ($date)"
echo "Frequency defined: $1"
socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 pty,raw,echo=0,link=/tmp/ttyv1 &
PID=$!



echo "Pipe for /tmp/ttyv0 PID($PID)"
arecord -c1 -r48000 -D hw:Loopback -fS16_LE - | sudo /home/pi/OrangeThunder/bin/OT -i /dev/stdin -s 6000 -p /tmp/ttyv1 -f "$1" -a

echo "Removing /tmp/ttyv0 PI($PID)"
sudo pkill socat
#*-----------------------------------------------------------------------
#*                      End of Script 
#*-----------------------------------------------------------------------

