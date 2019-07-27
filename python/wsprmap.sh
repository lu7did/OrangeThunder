#!/bin/sh
cat WSPRData_Junio2019.txt | cut -f3- -d' ' | sort -k 1 | python wsprmap.py

