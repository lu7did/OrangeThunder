#!/usr/bin/python3
#*------------------------------------------------------------------------
#* pause utility
#* aid to manage input
#*------------------------------------------------------------------------
import select
import sys
if len(sys.argv) >= 2:
   st=""
   for x in range(1, len(sys.argv)):
       st=sys.argv[x]+" "
else:
   st="Press enter to continue:"
e=input(st).upper()
if e != "\n":
   print(e)
exit()

