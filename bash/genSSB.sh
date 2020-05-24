#!/bin/sh

arecord  -r48000 -D hw:Loopback,1,0 -fS16_LE -   | sudo genSSB  -x -v 2  -t 2 | sudo sendRF -i /dev/stdin -d  -s 6000 -f 14074000
