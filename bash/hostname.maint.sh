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
BCN=`hostname`".bcn"
TLM=`hostname`".tlm"
MAILBOX="/var/mail/pi"
WSPR="/home/pi/WsprryPi/"
ROLL=`hostname`".maint.log"

echo "-------------------------------------------------------" 2>$BPATH$ERR >$BPATH$LOG
echo "Maintenance "$0 "hostname("`hostname`") epoch("`date`")" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Removing oldest version" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r $BPATH$BUP"(3)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Rotating previous versions" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (2) to (3)" 2>> $BPATH$ERR >>$BPATH$LOG
sudo cp $BPATH$BUP"(2)" $BPATH$BUP"(3)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (1) to (2)" 2>> $BPATH$ERR >>$BPATH$LOG
sudo cp $BPATH$BUP"(1)" $BPATH$BUP"(2)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Version (*) to (1)" 2>> $BPATH$ERR >>$BPATH$LOG
sudo cp $BPATH$BUP $BPATH$BUP"(1)" 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Remove last backup" 2>> $BPATH$ERR >>$BPATH$LOG
sudo -rm -r $BPATH$BUP  2>>$BPATH$ERR >>$BPATH$LOG

echo " " 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Executing backup path("$APATH") at ("$BPATH$BUP")" 2>>$BPATH$ERR >>$BPATH$LOG
sudo tar -zcf $BPATH$BUP $APATH 2>>$BPATH$ERR >>$BPATH$LOG

sudo tar -tf $BPATH$BUP 2>>$BPATH$ERR >>$BPATH$LST
echo `date`" Backup finished, backup " `sudo cat $BPATH$LST | wc -l`" files. Content list is "$BPATH$LST 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Removing mailbox Executing backup pat("$MAILBOX")" 2>>$BPATH$ERR >>$BPATH$LOG
sudo rm -r $MAILBOX 2>>$BPATH$ERR >>$BPATH$LOG

echo `date`" Pruning Maintenance Logs("$ROLL")" 2>>$BPATH$ERR >>$BPATH$LOG

sudo cat $WSPR"whisper.log" > $WSPRlogs/"whisper_"$(date -d yesterday '+%Y%m%d').log
sudo cat /dev/null > $WSPR"whisper.log"

sudo cat $WSPR"wsprRxTx.log" > $WSPRlogs/"wsprRxTx_"$(date -d yesterday '+%Y%m%d').log
sudo cat /dev/null > $WSPR"whisper.tlm"

sudo cat $BPATH$ROLL > $WSPRlogs/$ROLL"_"$(date -d yesterday '+%Y%m%d').log
sudo cat /dev/null > $BPATH$ROLL

exit 0
