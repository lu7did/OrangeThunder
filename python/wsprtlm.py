#!/bin/phyton
# -*- coding: latin-1 -*-
from __future__ import with_statement
#*-------------------------------------------------------------------------
#* wsprtlm.py
#* Compute a WSPR frame based on telemetry data
#*-------------------------------------------------------------------------
#// License:
#//   This program is free software: you can redistribute it and/or modify
#//   it under the terms of the GNU General Public License as published by
#//   the Free Software Foundation, either version 2 of the License, or
#//   (at your option) any later version.
#//
#//   This program is distributed in the hope that it will be useful,
#//   but WITHOUT ANY WARRANTY; without even the implied warranty of
#//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//   GNU General Public License for more details.
#//
#//   You should have received a copy of the GNU General Public License
#//   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#// lu7did: initial load
#*--------------------------------------------------------------------------------
#* Import libraries
#*--------------------------------------------------------------------------------
import zipfile
import os
import glob
import sys
import numpy as np
import math
import argparse

#*------------------------------------------------------------------------------
#* Default definitions
#*------------------------------------------------------------------------------
qthloc="GF05te"
h=30
bat=1.3
temp=67.0
channel=0
speed=0
gps=1
sats=1

#*------------------------------------------------------------------------------
#* Define power table
#*------------------------------------------------------------------------------
pwrlist=[0,3,7,10,13,17,20,23,27,30,33,37,40,43,47,50,53,57,60]

#*--------------------------------------------------------------------------------------
#* Compute callsign data element
#*--------------------------------------------------------------------------------------
def encode_callsign(qthloc,height,channel):
   
   sL1=qthloc[4].upper()
   sL2=qthloc[5].upper()
   L1=ord(sL1)
   L2=ord(sL2)
   L1b=L1-ord("A")
   L2b=L2-ord("A")
   i=24*L1b+L2b
   hint=int(h/20)


   x3=1
   x2=x3*26
   x1=x2*26
   x0=x1*26

   chn=channel%10

   f0=1068*i+hint
   d0=int(f0/x0)

   f1=f0-x0*d0
   d1=int(f1/x1)

   f2=f1-x1*d1
   d2=int(f2/x2)

   f3=f2-x2*d2
   d3=int(f3/x3)

#*------------------------------------------------------------------------------------
#* Encode into CALLSIGN field
#*------------------------------------------------------------------------------------
   
   sC="------"
   if chn<10:
      sC="0"+sC[1:]
   else:
      sC="Q"+sC[1:]

   if d0<10:
      sC=sC[:1]+chr(d0+ord("0"))+sC[2:]
   else:
      sC=sC[:1]+chr((d0-10)+ord("A"))+sC[2:]

   r1=int(chn%10)
   sC=sC[:2]+chr(r1+ord("0"))+sC[4:]
   sC=sC[:3]+chr(d1+ord("A"))+sC[5:]
   sC=sC[:4]+chr(d2+ord("A"))+sC[6:]
   sC=sC[:5]+chr(d3+ord("A"))+sC[7:]

   return sC


#*-----------------------------------------------------------------------------------
#* Encode battery, temperature, GPS (not implemented) and satellite (not implemented)
#*-----------------------------------------------------------------------------------
def encode_locator(temp,bat,gps,sat):

   k1=1024/90
   k2=1024/40

   z0=90
   z1=40
   z2=42
   z3=2
   z4=2

   v0a=int(1024*(temp*0.01+2.73)/5)
   v0b=int((v0a-457)/2)
   v0c=v0b

   v1a=int(1024*bat/5)
   v1b=int(round((v1a-614)/10,0))
   v1c=(v0c*z1)+v1b

   v2a=speed
   v2b=int(v2a/2)
   v2c=v1c*z2+v2b

   v3a=gps
   v3b=v3a
   v3c=v2c*z3+v3b

   v4a=sats
   v4b=v4a
   v4c=v3c*z4+v4b

   x3=18*10*10*19
   x2=10*10*19
   x1=10*19
   x0=19

   z3=v4c
   w3=int(z3/x3)

   z2=z3-(x3*w3)
   w2=int(z2/x2)

   z1=z2-(x2*w2)
   w1=int(z1/x1)

   z0=z1-(x1*w1)
   w0=int(z0/x0)

   wpwr=z0-(x0*w0)
   sG="----"

   sG=chr(w3+ord("A"))+sG[1:]
   sG=sG[:1]+chr(w2+ord("A"))+sG[2:]
   sG=sG[:2]+str(w1)+sG[3:]
   sG=sG[:3]+str(w0)+sG[4:]
   if args.q == True:
      if args.v == True:
         print("Telemetry locator: %s" % sG)
   return sG,pwrlist[wpwr]

#*-------------------------------------------------------------------------------
#* Decode CALLSIGN into locator and height
#*-------------------------------------------------------------------------------
def decode_CALLSIGN(callsign):

   if callsign[2] == "Q":
      k1=10
   else:
      k1=0

#   print("multiplier k1=%d" % k1)
   
   k2=((ord(callsign[1]))-ord("A")+10)*26*26*26
   k2=k2+26*26*( ord(callsign[3])-ord("A") )
   k2=k2+26*(ord(callsign[4])-ord("A"))
   k2=k2+ord(callsign[5])-ord("A")
   
   f1=k2
   d1=int(f1/(24*1068))
   c1=chr(ord(chr(d1))+ord("A"))
#   print("First character f1=%d d1=%d c1=%s" % (f1,d1,c1))

   f2=f1-d1*(24*1068)
   d2=int(f2/1068)
   c2=chr(ord(chr(d2))+ord("A"))
#   print("Second character f2=%d d2=%d c2=%s" % (f2,d2,c2))

   f3=f2-d2*1068
   d3=f3*20
   c3=str(d3)
#   print("Third character f3=%d d3=%d c3=%s" % (f3,d3,c3))

#   print("Locator %s%s Height %s" % (c1,c2,c3))       

   return c1,c2,c3


#
#SI(ESERROR(VALOR(F5));
#  10+CODIGO(F5)-CODIGO("A");
#  VALOR(F5))+26*26*(CODIGO(H5)-CODIGO("A"))+26*(CODIGO(I5)-CODIGO("A"))+(CODIGO(J5)-CODIGO("A")

#*-------------------------------------------------------------------------------
#* Decode CALLSIGN into locator and height
#*-------------------------------------------------------------------------------
def decode_LOCATOR_PWR(locator,power):

   t=0.0
   v=0.0
   gps=0
   sat=0
   k1=0

   f1=(ord(locator[0])-ord("A"))
   k1=f1*19*18*10*10
   print("Factor 1 f1 %d k1 %d" % (f1,k1))

   
   f2=(ord(locator[1])-ord("A"))
   k2=f2*10*10*19
   print("Factor 2 f2 %d k2 %d" % (f2,k2))
  

   f3=(int(locator[2]))
   k3=f3*19*10
   print("Factor 3 f3 %d k3 %d" % (f3,k3))

   f4=2
   k4=f4

   k=k1+k2+k3+k4
   print("Factor Total %d" % k)

   d1=int(k/(40*42*2*2))
   e1=d1*2+457
   g1=e1*5/1024
   h1=100*(g1-2.73)

   print("d1=%d e1=%d g1=%d temp=%d" % (d1,e1,g1,h1))


   return str(t),str(v),str(gps),str(sat)

#*-------------------------------------------------------------------------------
#* Start Processing
#*-------------------------------------------------------------------------------

p = argparse.ArgumentParser()
p.add_argument('-g', help="Grid Locator XX00XX")
p.add_argument('-v', help="Verbosity active",action="store_true")
p.add_argument('-e', help="Elevation above ground [mts]")
p.add_argument('-c', help="Telemetry channel [0..19]")
p.add_argument('-b', help="Battery level [volts]")
p.add_argument('-t', help="Temperature [C]")
p.add_argument('-q', help="Encode battery, temp and other as locator",action="store_true")
p.add_argument('-l', help="Encode locator and height into callsign",action="store_true")
p.add_argument('-p', help="Encode others as power",action="store_true")

args = p.parse_args()
if args.v==True:
   print("Grid locator:%s" % (args.g))
   print("Elevation above ground: %s" % (args.e))
   print("Telemetry channel: %s" % (args.c))
   print("Battery level: %s" % (args.b))
   print("Temperature: %s" % (args.t))

if (args.l == False) and (args.p == False) and (args.q == False) :
   print("Either -l or -p or -q needs to be selected, type wsprtlm.py -h for help")
   exit()

if (args.g == None) or (args.e == None) or (args.b == None) or (args.t == None) or (args.c == None) :
   print("One or more parameters missing, type wsprtlm.py -h for help")
   exit()   

#*--------------------------------------------------------------------------------------
#* Assign variables
#*--------------------------------------------------------------------------------------
qthlog=args.g
h=float(args.e)
bat=float(args.b)
temp=float(args.t)
channel=int(args.c)

#*--------------------------------------------------------------------------------------
#* Validate arguments
#*--------------------------------------------------------------------------------------
if len(qthloc) <= 5 :
   print("QTH Loc %s has fewer than 6 positions, type wsprtlm.py -h for help" % (qthloc))
   exit()

#*-------------------------------------------------------------------------------
#* Process telemetry into callsign
#*-------------------------------------------------------------------------------
if args.l == True:
   LIC=encode_callsign(qthloc,h,channel)
   if args.v == True:
      print("Telemetry Locator and normalized height into CALLSIGN: %s" % (LIC))
   else:
      sys.stdout.write("%s " % LIC)

if args.q == True:
   GRID,PWR=encode_locator(temp,bat,1,1)

   if args.v == True:
      print("Telemetry Temperature, Battery and others: %s %s" % (GRID,PWR))
   else:
      sys.stdout.write("%s " % (GRID))
#*-------------------------------------------------------------------------------
#* Process (fake) power indicator
#*-------------------------------------------------------------------------------

if args.p == True:
   sys.stdout.write("7")

#print(" ")
#print("decode_CALLSIGN(%s%s %s)" % decode_CALLSIGN(LIC))
#print("decode_LOCATOR_POWER(%s %s %s %s)" % decode_LOCATOR_PWR(GRID,PWR))

exit()
