#!/bin/sh

cpu='acrux bcrux gacrux kcrux ecrux mucrux'
for i in $cpu
 do
    echo "Rebooting $i"  
    ssh pi@$i 'sudo shutdown -h now'
done

#ssh pi@acrux 'sudo shutdown -h now'
#ssh pi@bcrux 'sudo shutdown -h now'
#ssh pi@gacrux 'sudo shutdown -h now'
#ssh pi@kcrux 'sudo shutdown -h now'
#ssh pi@ecrux 'sudo shutdown -h now'
