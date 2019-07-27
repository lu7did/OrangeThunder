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

storeFile /home/pi/whisper/wsprRxTx.log
storeFile /home/pi/whisper/whisper.log
storeFile /home/pi/.local/share/WSJT-X/ALL_WSPR.TXT

sudo cat /var/log/syslog > /home/pi/$(hostname).syslog
storeFile  /home/pi/$(hostname).syslog

cd $WAV
echo `date`" Pruning wsjtx .Wav/.C2 files" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r *.c2
for f in `ls -L *.wav | head -n 1000` ; do echo $f; sudo rm -r $f; done

exit 0
