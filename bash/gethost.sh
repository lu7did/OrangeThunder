#!/bin/sh

#*---------------------------------------------------------------------
#* gethost.sh
#* Backup critical personality files from a host
#*---------------------------------------------------------------------
tar  -vczf $(hostname).host.tar.gz -C / ~/gethost.sh ~/$(hostname).maint.sh /etc/motd /etc/hosts  ~/Downloads/*.jpg ~/*.sh ~/*.build ~/*.setup
echo " "
echo "***********************************************************************"
echo "* backup complete                                                     *"
echo "* to restore execute                                                  *"
echo "*     sudo tar -zxvf "$(hostname)".host.tar.gz -C /            *"
echo "***********************************************************************"
echo " "
