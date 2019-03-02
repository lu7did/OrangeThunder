#!/bin/sh

cpu='acrux bcrux gacrux kcrux ecrux mucrux magellan'
for i in $cpu
 do
    #p=$(ping -c 1 -t 1 $i | grep -a "icmp" | wc -l)
    p=$(ssh pi@$i './hostStatus.sh && ./hostCheck.sh' | tail -n 1)
    echo "TLM($i): $p"
done

#ssh pi@acrux 'sudo shutdown -h now'
#ssh pi@bcrux 'sudo shutdown -h now'
#ssh pi@gacrux 'sudo shutdown -h now'
#ssh pi@kcrux 'sudo shutdown -h now'
#ssh pi@ecrux 'sudo shutdown -h now'
