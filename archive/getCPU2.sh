#!/bin/bash


#*--------------- Requires apt-get install sysstat

sar 1 3 | grep "Media:" | while read a ; do
 echo $a | awk '{print $3 + $4 + $5 + $6 + $7}';
done
exit 0

