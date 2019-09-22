#!/bin/sh

#CPUList='acrux bcrux gacrux kcrux lcrux dcrux mucrux'
SWList='./sources.list'
for i in $(cat /etc/hosts | grep "192." | awk '{ print $2 ; }' | cut -f1 -d"%")
 do
    P=`ping $i -c 1 | grep "time=" | wc -l`
    if [ $P -eq 1 ]
    then
       echo "Host $i active"

       for j in $SWList
       do
         echo "Transferring $j to host $i"
         p=$(scp ./sources.list pi@$i:/home/pi/sources.list && ssh pi@$i  'sudo cp /home/pi/sources.list /etc/apt/sources.list')
         echo "Host($i) Package($j) $p transferred"
       done
    fi
done

