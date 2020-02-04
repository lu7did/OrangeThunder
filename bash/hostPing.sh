#!/bin/sh
#*----------------------------------------------------------------------------
#* hostPing
#* Check if all nodes in the local network are alive
#*----------------------------------------------------------------------------
for i in $(cat /etc/hosts | grep "192." | awk '{ print $2 ; }' | cut -f1 -d"%")
 do
    RESP=$( ping -c 1 -t 1 $i | grep -a "time=")
    ALIVE=$( ping -c 1 -t 1 $i | grep -a "time=" | wc -l)
    IP=$(ping -c 1 -t 1 $i | grep -a ":" | cut -f2 -d"(" | cut -f1 -d")")
    if [ $ALIVE -eq 1 ]
    then
       echo "$(date) {$IP} $i Response[$(echo $RESP | awk '{ print $8 ; } ' | cut -f2 -d"=") msecs]"
    else
        echo "$(date) {$IP} $i (OFFLINE)"
    fi 
done

