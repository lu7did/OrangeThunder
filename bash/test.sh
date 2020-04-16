#!/bin/sh


if [ "$(cat counter.txt) % 10" == 0 ] 
then
   echo "salio por si"
else
   echo "salio por no"
fi
echo "$(cat counter.txt)+1" > counter.txt

