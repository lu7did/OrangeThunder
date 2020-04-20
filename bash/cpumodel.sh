#!/bin/sh
# Raspberry Pi Zero
#  900092 900093 9000C1
cat /proc/cpuinfo | grep 'Revision' | awk '{print $3}'
