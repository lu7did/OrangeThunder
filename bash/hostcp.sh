#!/bin/sh
echo "copying file $1"
for i in $(cat /etc/hosts | grep "192." | awk '{ print $2 ; }' | cut -f1 -d"%")
 do
    if [ "$i" != "$(hostname)" ] && [ "$i" != "icrux" ]; then
       echo "Checking host $i"
       P=`ping $i -c 1 | grep "time=" | wc -l`
       if [ $P -eq 1 ]
       then
          echo "Host $i active"
          echo "Copying file $1 to at host $i"
          scp $1 pi@$i:/home/pi/Downloads/$1 
          ssh pi@$i "sudo cp /home/pi/Downloads/$1 $2/$1 && rm -r /home/pi/Downloads/$1"

          echo "Host($i) file copied"
       fi
    else
       echo "Host ($i) skipped."
    fi

done

