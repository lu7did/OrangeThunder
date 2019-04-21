#!/bin/sh

cpu='acrux'
for i in $cpu
 do
    echo "Installing watchdog at  $i"  
    ssh pi@$i 'cd /home/pi/OrangeThunder && sudo git pull && cd /home/pi && cp ./OrangeThunder/bash/watchdog.setup . && sudo chmod +x watchdog.setup && ./watchdog.setup'
done

#ssh pi@acrux 'sudo shutdown -h now'
#ssh pi@bcrux 'sudo shutdown -h now'
#ssh pi@gacrux 'sudo shutdown -h now'
#ssh pi@kcrux 'sudo shutdown -h now'
#ssh pi@ecrux 'sudo shutdown -h now'
