#!/bin/sh

find /path/to/files/ -type f -name '*.jpg' -mtime +30 -exec rm {} \;
