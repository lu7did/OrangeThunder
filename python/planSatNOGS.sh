#!/bin/sh
#*--------------------------------------------------------------------
#* planSatNOGS.sh
#*
#* Script to load SatNOGS observations automatically
#*
#*--------------------------------------------------------------------
ID=499
TIME=24.0
PRIORITY="priorities_$ID.txt"
TRANSMITTERS="/tmp/cache/transmitters_$ID.txt"
OPS=" -f -l INFO"
MODES="CW"
NORAD=" 20442 "
EXCLUIDO="43466 39430"
AZMIN=" --azmin 340 "
AZMAX=" --azmax 20 "
echo "*********************"
echo "* Set priorities    *"
echo "*********************"
rm -r $PRIORITY

#*---- Include satellites by mode
for mode in $MODES; do
    echo "Selecting priority for Mode($mode)"
    awk '{if ($3>=80) print $0 }' $TRANSMITTERS | grep -e $mode | awk '{printf("%s 1.0 %s\n",$1,$2)}'  | tee -a $PRIORITY
done

#*---- Include satellites despite of the mode by NORAD Id
for norad in $NORAD; do
    echo "Selecting priority for Satellite($NORAD)"
    awk '{if ($3>=80) print $0 }' $TRANSMITTERS | grep -e $norad | awk '{printf("%s 1.0 %s\n",$1,$2)}'  | tee -a $PRIORITY
done

#*---- Remove unwanted satellites
for excl in $EXCLUIDO; do
   echo "Excluding satellite $excl"
   awk '!/$excl/' priorities_499.txt > temp && mv temp priorities_499.txt
done

echo "$(date) planSatNOGS: $(cat $PRIORITY | wc -l) satellites prioritized" | logger -i -t "$ID"


echo "*********************"
echo "* Scheduling passes *"
echo "*********************"
python /home/pi/satnogs-auto-scheduler/schedule_single_station.py -s $ID -d $TIME -P $PRIORITY $AZMIN $AZMAX $OPS
exit 0
