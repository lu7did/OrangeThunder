#!/bin/bash

top -b -n2 | grep "Cpu(s)"|tail -n 1 | awk '{print $2 + $4}'

exit 0

