#!/bin/sh

CPUList='acrux bcrux gacrux kcrux ecrux mucrux magellan'
SWList='hostPing.sh hostShut.sh'
for i in $CPUList
 do
    for j in $SWList
    do
      echo "Transferring $j to host $i"
      p=$(ssh pi@$i 'cd /home/pi/OrangeThunder && sudo git pull && sudo cp -r /home/pi/OrangeThunder/bash/$j /home/pi/$j')
      echo "Host($i) Package($j) $p transferred"
    done
done

