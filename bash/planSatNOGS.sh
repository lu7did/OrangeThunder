#!/bin/sh
#*--------------------------------------------------------------------
#* planSatNOGS.sh
#*
#* Script to load SatNOGS observations automatically
#*
#*--------------------------------------------------------------------
ID=499
TIME=1.2
PRIORITY="priorities_$ID.txt"
TRANSMITTERS="/tmp/cache/transmitters_$ID.txt"
OPS=" -T -f -l INFO"
MODES="CW"
echo "*********************"
echo "* Set priorities    *"
echo "*********************"
rm -r $PRIORITY
for mode in $MODES; do
    echo "Selecting priority for Mode($mode)"
    awk '{if ($3>=80) print $0 }' $TRANSMITTERS | grep -e $mode | awk '{printf("%s 1.0 %s\n",$1,$2)}' | tee -a $PRIORITY
done
echo "*********************"
echo "* Scheduling passes *"
echo "*********************"
python /home/pi/satnogs-auto-scheduler/schedule_single_station.py -s $ID -d $TIME -P $PRIORITY $OPS 

exit 0
