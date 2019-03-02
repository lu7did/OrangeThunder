#!/bin/sh

cpu='acrux bcrux gacrux kcrux ecrux mucrux magellan'
for i in $cpu
 do
    echo "Ping to $i"  
    ping -c 1 -t 1 $i | grep -a "icmp"
done

#ssh pi@acrux 'sudo shutdown -h now'
#ssh pi@bcrux 'sudo shutdown -h now'
#ssh pi@gacrux 'sudo shutdown -h now'
#ssh pi@kcrux 'sudo shutdown -h now'
#ssh pi@ecrux 'sudo shutdown -h now'
