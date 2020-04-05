#!/bin/sh
socat -d -d -d PTY,raw,echo=0,link=/tmp/ttyv1 TCP:127.0.0.1:8080 

