#!/bin/sh

sudo cat /var/log/syslog | tail -n 5000  | grep -a "TLM"
