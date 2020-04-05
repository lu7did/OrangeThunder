#!/bin/sh
sudo socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 pty,raw,echo=0,link=/tmp/ttyv1 &
#sudo chmod a+rw /tmp/ttyv1
#*-----------------------------------------------------------------------
#*                      End of Script 
#*-----------------------------------------------------------------------

