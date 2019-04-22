#!/bin/sh
#*----------------------------------------------------------------------------
#* hostTelemetry
#* Check telemetry on all active nodes
#*----------------------------------------------------------------------------
for i in $(cat /etc/hosts | grep "192." | awk '{ print $2 ; }' | cut -f1 -d"%")
 do
    RESP=$( ping -c 1 -t 1 $i | grep -a "time=")
    ALIVE=$( ping -c 1 -t 1 $i | grep -a "time=" | wc -l)
    if [ $ALIVE -eq 1 ]
    then
       p=$(ssh pi@$i './hostStatus.sh && ./hostCheck.sh' | tail -n 1)
       echo "$p"

    else
        echo "$(date) $i (OFFLINE)"
    fi
done
