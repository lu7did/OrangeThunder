#!/bin/sh

### BEGIN INIT INFO
# Provides:        acrux.bup.sh
# Required-Start:  $network $remote_fs $syslog
# Required-Stop:   $network $remote_fs $syslog
# Default-Start:   2 3 4 5
# Default-Stop: 
# Short-Description: Start the acrux daily maintenance
### END INIT INFO

#*--------------------------------------------------------------------------------------------
#* Copy a file, place over the previous if it exists
#*--------------------------------------------------------------------------------------------
copyFile() {
echo `date`"copyFile: [$(hostname)] $1" 2>&1 >>$BPATH$LOG
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh mkdir /SouthernCroix/$(hostname)
./dropbox_uploader.sh upload $1 /SouthernCroix/$(hostname)/$1

}
#*--------------------------------------------------------------------------------------------
#* Store a file, create a daily version to preserver previous
#*--------------------------------------------------------------------------------------------
storeFile() {
echo `date`" storeFile: [$(hostname)] $1" 2>&1 >>$BPATH$LOG
F=$(date -d yesterday '+%Y%m%d').log
cd /home/pi/Dropbox-Uploader
./dropbox_uploader.sh upload $1 /SouthernCroix/$(hostname)/$1.$F
}
#*---------------------------------------------------------------------------------------------
#* move a file, same as store but deleting the source file afterwards
#*---------------------------------------------------------------------------------------------
moveFile() {
echo `date`" moveFileFile: [$(hostname)] $1" 2>&1 >>$BPATH$LOG
storeFile $1
sudo rm -r $1

}

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

copyFile $BPATH$BUP"(3)"

cd /home/pi
echo `date`" Removing oldest version" 2>&1 >>$BPATH$LOG
sudo rm -r $BPATH$BUP"(3)" 2>&1 >>$BPATH$LOG

echo `date`" Rotating previous versions" 2>&1 >>$BPATH$LOG

echo `date`" Version (2) to (3)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP"(2)" $BPATH$BUP"(3)" 2>>$BPATH$ERR >>$BPATH$LOG#

echo `date`" Version (1) to (2)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP"(1)" $BPATH$BUP"(2)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (*) to (1)" 2>&1 >>$BPATH$LOG
sudo cp $BPATH$BUP $BPATH$BUP"(1)" 2>&1 >>$BPATH$LOG

echo `date`" Remove last backup" 2>&1 >>$BPATH$LOG
sudo -rm -r $BPATH$BUP  2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Executing backup path("$APATH") at ("$BPATH$BUP")" 2>&1 >>$BPATH$LOG
sudo tar -zcf $BPATH$BUP $APATH 2>&1 >>$BPATH$LOG

sudo tar -tf $BPATH$BUP 2>&1 >>$BPATH$LST
echo `date`" Backup finished, backup " `sudo cat $BPATH$LST | wc -l`" files. Content list is "$BPATH$LST 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Removing mailbox Executing backup pat("$MAILBOX")" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r $MAILBOX 2>&1 >>$BPATH$LOG

echo `date`" Pruning Logs("$WSPR")" 2>&1 >>$BPATH$LOG

F=$(date -d yesterday '+%Y%m%d').log
echo `date`" Datum for logs is $F" 2>&1 >>$BPATH$LOG

storeFile /home/pi/whisper/wsprRxTx.log
storeFile /home/pi/whisper/whisper.log
storeFile /home/pi/.local/share/WSJT-X/ALL_WSPR.TXT
copyFile /home/pi/$(hostname).host.tar.gz

sudo cat /var/log/syslog > /home/pi/syslog
storeFile  /home/pi/syslog

cd $WAV
echo `date`" Pruning wsjtx .Wav/.C2 files" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r *.c2
for f in `ls -L *.wav | head -n 1000` ; do echo $f; sudo rm -r $f; done

exit 0
