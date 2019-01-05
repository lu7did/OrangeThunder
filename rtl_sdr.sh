#!/bin/sh
rtl_sdr -s 1200000 -f 14100000 -D 2 - | csdr convert_u8_f | ncat -4l 4952 -k --send-only --allow 127.0.0.1

