#!/bin/sh
SWList='./sources.list'
for i in $(cat /etc/hosts | grep "192." | awk '{ print $2 ; }' | cut -f1 -d"%")
 do
    P=`ping $i -c 1 | grep "time=" | wc -l`
    if [ $P -eq 1 ]
    then
       echo "Host $i active"
       echo "Executing command at host $i"
       ssh pi@$i 'cd OrangeThunder && sudo git pull && sudo apt-get update'
       echo "Host($i) executed"
    fi
done

