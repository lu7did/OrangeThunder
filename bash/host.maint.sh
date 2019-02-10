#!/bin/sh

### BEGIN INIT INFO
# Provides:        acrux.bup.sh
# Required-Start:  $network $remote_fs $syslog
# Required-Stop:   $network $remote_fs $syslog
# Default-Start:   2 3 4 5
# Default-Stop: 
# Short-Description: Start the acrux daily maintenance
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

APATH="/home/"
BPATH="/var/backups/"
BUP=`hostname`".tgz"
LOG=`hostname`".log"
ERR=`hostname`".err"
LST=`hostname`".lst"
MAILBOX="/var/mail/pi"
WSPR="/home/pi/whisper/"
ROLL=`hostname`".maint.log"
WAV="/home/pi/.local/share/WSJT-X/save/"

echo "-------------------------------------------------------" 2>&1 >$BPATH$LOG
echo "Maintenance "$0 "hostname("`hostname`") epoch("`date`")" 2>&1 >>$BPATH$LOG
cd /home/pi/Dropbox-Uploader

./dropbox_uploader.sh mkdir /SouthernCroix/$(hostname)
./dropbox_uploader.sh upload $BPATH$BUP"(3)" /SouthernCroix/$(hostname)/$BUP"(3)"

cd /home/pi
echo `date`" Removing oldest version" 2>&1 >>$BPATH$LOG
sudo rm -r $BPATH$BUP"(3)" 2>&1 >>$BPATH$LOG

echo `date`" Rotating previous versions" 2>&1 >>$BPATH$LOG

echo `date`" Version (2) to (3)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP"(2)" $BPATH$BUP"(3)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (1) to (2)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP"(1)" $BPATH$BUP"(2)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (*) to (1)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP $BPATH$BUP"(1)" 2>&1 >>$BPATH$LOG

echo `date`" Remove last backup" 2>&1 >>$BPATH$LOG
sudo -rm -r $BPATH$BUP  2>>$BPATH$ERR >>$BPATH$LOG

echo " " 2>&1 >>$BPATH$LOG

echo `date`" Executing backup path("$APATH") at ("$BPATH$BUP")" 2>&1 >>$BPATH$LOG
sudo tar -zcf $BPATH$BUP $APATH 2>&1 >>$BPATH$LOG

sudo tar -tf $BPATH$BUP 2>&1 >>$BPATH$LST
echo `date`" Backup finished, backup " `sudo cat $BPATH$LST | wc -l`" files. Content list is "$BPATH$LST 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Removing mailbox Executing backup pat("$MAILBOX")" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r $MAILBOX 2>&1 >>$BPATH$LOG

echo `date`" Pruning Logs("$WSPR")" 2>&1 >>$BPATH$LOG

F=$(date -d yesterday '+%Y%m%d').log
echo `date`" Datum for logs is $F" 2>&1 >>$BPATH$LOG


sudo cat /home/pi/whisper/"wsprRxTx.log"  > /home/pi/wsprRxTx.$F 2> /dev/null
sudo rm -r /home/pi/whisper/"wsprRxTx.log" 2>>$BPATH$ERR > /dev/null
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload /home/pi/wsprRxTx.$F /SouthernCroix/$(hostname)/$(hostname).wsprRxTx.$F
cd /home/pi

sudo cat  /home/pi/whisper/"whisper.log" > /home/pi/whisper.$F  2>&1 >>$BPATH$LOG
sudo rm -r /home/pi/whisper/"whisper.log" 2>>$BPATH$ERR
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload /home/pi/whisper.$F /SouthernCroix/$(hostname)/$(hostname).whisper.$F

cd /home/pi
sudo cat  /home/pi/.local/share/WSJT-X/ALL_WSPR.TXT > /home/pi/ALL_WSPR.$F  2>&1 >>$BPATH$LOG
sudo rm -r  /home/pi/.local/share/WSJT-X/ALL_WSPR.TX  2>>$BPATH$ERR
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload /home/pi/ALL_WSPR.$F /SouthernCroix/$(hostname)/$(hostname).ALL_WSPR.$F
cd /home/pi

cd $WAV
echo `date`" Pruning wsjtx .Wav/.C2 files" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r *.c2
for f in `ls -L *.wav | head -n 1000` ; do echo $f; sudo rm -r $f; done


cd /home/pi
echo `date`" Preserving $(hostname) personality" 2>>$BPATH$ERR >>$BPATH$LOG
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload /home/pi/$(hostname).host.tar.gz /SouthernCroix/$(hostname)/$(hostname).host.tar.gz

cd /home/pi

sudo cat /var/log/syslog > /home/pi/syslog.$F 2>> /dev/null
sudo cat /dev/null > /var/log/syslog
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload /home/pi/syslog.$F /SouthernCroix/$(hostname)/syslog.$F
cd /home/pi

sudo rm -r /home/pi/ALL_WSPR.$F
sudo rm -r /home/pi/$(hostname).host.tar.gz
sudo rm -r /home/pi/syslog.$F
sudo rm -r /home/pi/whisper.$F
sudo rm -r /home/pi/wsprRxTx.$F
exit 0
