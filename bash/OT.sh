#!/bin/sh
#*-----------------------------------------------------------------------
#* OT.sh
#* Script to implement a SSB transceiver using the OrangeThunder program
#* A remote pipe is implemented to carry CAT commands
#* Sound is feed thru the arecord command (PulseAudio ALSA), proper hardware
#* interface needs to be established. (-a parameter enables VOX)
#* the script needs to be called with the frequency in Hz as a parameter
#*            ./OT.sh [frequency in Hz]
#*-----------------------------------------------------------------------
clear
echo "Orange Thunder based SSB transceiver ($date)"
echo "Frequency defined: $1"
if [ "$1" -eq "" ]; then
   echo "Operation frequency must be informed, try:"
   echo "./OT.sh [frequency in Hz]"
   exit 16
fi
#*----------------------------------------*
#* Install ALSA loopback on Kernel        *
#*----------------------------------------*
sudo modprobe snd-aloop
#*----------------------------------------*
#* Launching socat server                 *
#*----------------------------------------*
#socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 pty,raw,echo=0,link=/tmp/ttyv1 &
#PID=$!
#echo "CAT commands piped from apps thru /tmp/ttyv1 PID($PID)"
sudo socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 TCP-LISTEN:8080 &
#sudo rigctld --model=$(rigctl -l | grep "FT-817" | awk '{print $1}') -r /tmp/ttyv0 -t 4532 -s 4800 -$
#RIGCTL=$!
#echo "rigctld PID($RIGCTL)"

#*----------------------------------------*
#* Transceiver execution using loopback   *
#*----------------------------------------*
arecord -c1 -r48000 -D hw:Loopback -fS16_LE - | sudo /home/pi/OrangeThunder/bin/OT -i /dev/stdin -s 6000 -p /tmp/ttyv0 -f "$1" -a
#*----------------------------------------*
#* Transceiver execution using loopback   *
#*----------------------------------------*

#echo "Killing rigtcl PID($RIGCTL)"
#sudo pkill rigctld


#echo "Removing /tmp/ttyv0 PI($PID)"
#sudo pkill socat
#*----------------------------------------*
#*           End of Script                * 
#*----------------------------------------*

