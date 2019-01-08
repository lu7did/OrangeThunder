#!/bin/python3
#// cat817 demo program for FT 817 CAT emulator library
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
#*----------------------------------------------------------------------------
#* Initialization
#* DO NOT RUN EITHER AS A rc.local script nor as a systemd controlled service
#*----------------------------------------------------------------------------
#*-----------------------------------------------------------------------------$
#* Import libraries
#*-----------------------------------------------------------------------------$
import serial
import zipfile
import os
import glob
import sys
import numpy as np
import math
import argparse
import subprocess
import time
import datetime
import binascii
#*----------------------------------------------------------------------------
#* Transceiver mode variables
#*----------------------------------------------------------------------------
ft817_modes={ 0x00 : 'LSB',
              0x01 : 'USB',
              0x02 : 'CW',
              0x03 : 'CWR',
              0x04 : 'AM',
              0x08 : 'FM',
              0x0A : 'DIG',
              0x0C : 'PKT'}

#*----------------------------------------------------------------------------
#* Transceiver state variables
#*----------------------------------------------------------------------------
fVFOA=14074200
fVFOB= 7000000
vfoAB=0
SPLIT=False
PTT=False
CAT=4800
MODE=01
LOCK=False
CLAR=False
#*----------------------------------------------------------------------------
#* Function definitions
#*----------------------------------------------------------------------------


def log(logText):
    print("%s:%s %s" % (sys.argv[0],time.ctime(),logText))

def getVFO(v):
    return chr(ord("A")+v)

def spawn(cmd):
    if (args.v==True):
       log("spawn: %s" % cmd)
    z=subprocess.Popen(cmd)
    return z

#*-----------------------------------------------------------------------------$
#* Execute a command and return the result
#*-----------------------------------------------------------------------------$
def execute(cmd):
    """
        Purpose  : To execute a command and return exit status
        Argument : cmd - command to execute
        Return   : exit_code
    """
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (result, error) = p.communicate()

    rc = p.wait()

    if rc != 0:
        log("Error: failed to execute command: %s" % cmd)
        log(error)
    return result
#*-----------------------------------------------------------------------
#* printBuffer
#* used to dump content of byte array used for CAT commands and responses
#*-----------------------------------------------------------------------
def printBuffer(rx):
    i=0
    o=''
    while i<len(rx):
      j=rx[i]
      h=hex(j)[2:].zfill(2)
      o=o+' '+h
      i=i+1
    return o
#*---------------------------------------------------------------------------
#* BCD2Hex
#* Convert nibble
#*---------------------------------------------------------------------------
#def BCD2Hex(f0,f1,f2,f3):
#    return '%s %s %s %s' % (hex(f0)[2:0],hex(f1)[2:0],hex(f2)[2:],hex(f3)[2:0])
#*---------------------------------------------------------------------------
#* bcdToDec
#* Convert nibble
#*---------------------------------------------------------------------------
def bcdToDec(val):
  return  (val/16*10) + (val%16) 
#*--------------------------------------------------------------------------
#* decToBcd
#* Convert nibble
#*-------------------------------------------------------------------------
def decToBcd(val):
  return  (val/10*16) + (val%10)

#*----------------------------------------------------------------------
#* Decode integer into BCD
#* Frequency is expressed as / 10
#*----------------------------------------------------------------------
def dec2BCD(f):

  fz=int(f/10)
  f0=int(fz/1000000)
  
  x1=fz-f0*1000000
  f1=int(x1/10000)
  
  x2=x1-f1*10000
  f2=int(x2/100)

  x3=x2-f2*100
  f3=int(x3)

  f0=decToBcd(f0)
  f1=decToBcd(f1)
  f2=decToBcd(f2)
  f3=decToBcd(f3)

  if (args.v==True):
     log('dec2BCD: f(%d)->f(%d) %x %x %x %x' % (f,fz,f0,f1,f2,f3))
  return bytearray([f0,f1,f2,f3,0x00])
#*-----------------------------------------------------------------------
#* BCD2Dec
#* Convert frequency /10 from BCD to integer
#*-----------------------------------------------------------------------
def BCD2Dec(rxBuffer):
    f=0
    f=f+bcdToDec(rxBuffer[0])*1000000
    f=f+bcdToDec(rxBuffer[1])*10000
    f=f+bcdToDec(rxBuffer[2])*100
    f=f+bcdToDec(rxBuffer[3])*1
    f=f*10
    if (args.v==True):
       log('BCD2Dec: Cmd[%s] f=%d' % (printBuffer(rxBuffer),f))
    return f
#*-----------------------------------------------------------------------
#* return value
#*-----------------------------------------------------------------------

def getFT817mode(m):
    return ft817_modes.get(m,default = None)

#*-----------------------------------------------------------------------
#* processFT817
#* main CAT command processor
#*----------------------------------------------------------------------
def processFT817(rxBuffer,n,s):

#*--- Required definition to avoid re-entrancy problems

    global vfoAB,fVFOA,fVFOB,PTT,CAT,MODE,SPLIT,LOCK

#*--- Process command chain

    if rxBuffer[4] == 0x01:    #*---- Set Frequency
       fx=bytearray([rxBuffer[0],rxBuffer[1],rxBuffer[2],rxBuffer[3]])
       f=BCD2Dec(fx)
       prevfVFOA=fVFOA
       prevfVFOB=fVFOB
       if vfoAB == 0:
          fVFOA =  int(f)
       else:
          fVFOB =  int(f)
       r=bytearray([0])
       s.write(r)
       log('CAT[0x01] [%s] VFO[%d/%d] set to VFO[%d/%d]' % (printBuffer(rxBuffer),prevfVFOA,prevfVFOB,fVFOA,fVFOB))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x03:            #*-- Query Frequency
       if vfoAB==0:
          r=dec2BCD(int(fVFOA))
          r[4]=MODE
          s.write(r)
       else:
          r=dec2BCD(int(fVFOB))
          r[4]=MODE
          s.write(r)
       log('CAT[0x03] [%s] VFO[%d/%d] resp[%s]' % (printBuffer(rxBuffer),fVFOA,fVFOB,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0xf7:           #*--- Read TX Status (logging filtered because of high volume
       STATUS=0x00
       if PTT == False:    #has been inverted
          STATUS = STATUS | 0b10000000
       if SPLIT == True:   #has been inverted 
          STATUS = STATUS | 0b00100000
       r=bytearray([STATUS])
       s.write(r)
       #if args.v == True:
       log('CAT[0xf7] [%s] resp[%s]' % (printBuffer(rxBuffer),printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0xe7:            #*--- RX Status (logging filtered because of high volume
       r=bytearray([0x00])
       s.write(r)
       if args.v == True:
          log('CAT[0xE7][%s] resp[%s]' % (printBuffer(rxBuffer),printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0


    if rxBuffer[4] == 0xBB:            #*--- Read EEPROM
       r=bytearray([0,0])
       s.write(r)
       if args.v == True:
          log('CAT[0xBB] *TEMP* [%s] resp[%s]' % (printBuffer(rxBuffer),printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x81:            #*--- Switch VFO A/B
       prevAB=vfoAB
       if vfoAB == 0:
          vfoAB = 1
       else:
          vfoAB = 0
       r=bytearray([0x00])
       s.write(r)
       log('CAT[0x81] [%s] VFO(%s->%s) resp[%s]' % (printBuffer(rxBuffer),getVFO(prevAB),getVFO(vfoAB),printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x00:            #*--- Lock status
       prevLOCK=LOCK
       if LOCK==True:
          r=bytearray([0xf0])
          LOCK=False
       else:
          r=bytearray([0])
          LOCK=True
       s.write(r)
       log('CAT[0x00] [%s] LOCK(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevLOCK,LOCK,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0


    if rxBuffer[4] == 0x02:            #*--- Split On
       prevSPLIT=SPLIT
       if SPLIT==True:
          r=bytearray([0xf0])
          SPLIT=False
       else:
          r=bytearray([0x00])
          SPLIT=True
       s.write(r)
       log('CAT[0x02] [%s] SPLIT(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevSPLIT,SPLIT,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x07:            #*--- Set operating mode
       prevMODE=MODE
       prevs=getFT817mode(MODE)
       nextMODE=rxBuffer[0]
       s=getFT817mode(nextMODE)
       if s==None:
          log('CAT[0x07] Invalid CAT Mode(%d), ignore' % (nextMODE))
          r=bytearray([0x00])
       else:   
          MODE=nextMODE
          r=bytearray([0x00])
       s.write(r)
       log('CAT[0x07] [%s] MODE(%d<%s>->%d<%s>)  resp[%s]' % (printBuffer(rxBuffer),prevMODE,prevs,MODE,s,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x08:            #*--- PTT ON and Status
       prevPTT=PTT
       PTT=True
       if prevPTT==True:
          r=bytearray([0xf0])
       else:
          r=bytearray([0x00])
       s.write(r)
       log('CAT[0x08] [%s] PTT(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevPTT,PTT,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if ((rxBuffer[4] == 0x09) or (rxBuffer[4] == 0x0A) or (rxBuffer[4] == 0x0B) or (rxBuffer[4]==0x0C) or (rxBuffer[4]==0x0F)):            #*--- Lock status
       log('CAT[NI] [%s] ignored' % (printBuffer(rxBuffer)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x10:            #*--- Read TX Keyed state (undoc)
       if PTT==True:
          r=bytearray([0xf0])
       else:
          r=bytearray([0x00])
       s.write(r)
       log('CAT[0x10] [%s] PTT(%s)  resp[%s]' % (printBuffer(rxBuffer),PTT,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x82:            #*--- Split Off
       prevSPLIT=SPLIT
       if SPLIT==True:
          r=bytearray([0x00])
       else:
          r=bytearray([0xf0])
       SPLIT=False
       s.write(r)
       log('CAT[0x82] [%s] SPLIT(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevSPLIT,SPLIT,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x85:            #*--- Clarifier Off
       prevCLAR=CLAR
       if CLAR==True:
          r=bytearray([0x00])
       else:
          r=bytearray([0xf0])
       CLAR=False
       s.write(r)
       log('CAT[0x85] [%s] CLARIFIER(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevCLAR,CLAR,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x05:            #*--- Clarifier On
       prevCLAR=CLAR
       if CLAR==True:
          r=bytearray([0xf0])
       else:
          r=bytearray([0x00])
       CLAR=True
       s.write(r)
       log('CAT[0x05] [%s] CLARIFIER(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevCLAR,CLAR,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if rxBuffer[4] == 0x88:            #*--- PTT OFF and Status
       prevPTT=PTT
       PTT=False
       if prevPTT==True:
          r=bytearray([0x00])
          #r=bytearray([0x0f])
       else:
          r=bytearray([0xf0])
          #r=bytearray([0x00])
       s.write(r)
       log('CAT[0x88] [%s] PTT(%s->%s)  resp[%s]' % (printBuffer(rxBuffer),prevPTT,PTT,printBuffer(r)))
       return  bytearray([0,0,0,0,0]),0

    if ((rxBuffer[4] == 0x8f) or (rxBuffer[4] == 0xa7) or (rxBuffer[4] == 0xba) or (rxBuffer[4]==0xbc) or (rxBuffer[4]==0xbd)):            #*--- Lock status
       log('CAT[N*] [%s] ignored' % (printBuffer(rxBuffer)))
       return  bytearray([0,0,0,0,0]),0

    if ((rxBuffer[4] == 0xbe) or (rxBuffer[4] == 0xf9) or (rxBuffer[4] == 0xf5)):            #*--- Lock status
       log('CAT[N-] [%s] ignored' % (printBuffer(rxBuffer)))
       return  bytearray([0,0,0,0,0]),0


#*----------------------------------------------------------------------------
#* MAIN PROGRAM
#*----------------------------------------------------------------------------

#*----------------------------------------------------------------------------
#* Process arguments
#*----------------------------------------------------------------------------
p = argparse.ArgumentParser()
p.add_argument('-i', help="Input serial port",default='/tmp/ttyv0')
p.add_argument('-o', help="Input serial port",default='/tmp/ttyv1')
p.add_argument('-r', help="Serial port rate",default=4800)
p.add_argument('-v', help="Verbose",action="store_true")

args = p.parse_args()
#*----------------------------------------------------------------------------
#* Start virtual port pair to honor CAT requests
#*----------------------------------------------------------------------------
z=subprocess.Popen('socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 pty,raw,echo=0,link=/tmp/ttyv1',shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
log('virtual port pair %s-%s enabled' % (args.i,args.o))
time.sleep(1)

#*----------------------------------------------------------------------------
#* Open listener side of virtual port pair
#*----------------------------------------------------------------------------
s=serial.Serial(args.o,args.r)
log('Source if %s, Sink %s rate(%d)' % (args.o,args.i,args.r))

CAT=args.r
vfoAB=0

#*----------------------------------------------------------------------------
#* Main receiving loop
#*----------------------------------------------------------------------------
o=''
n=0
vfoAB=0
rxBuffer=bytearray([0,0,0,0,0])

log('Main: Starting process current VFO is %s' % getVFO(vfoAB))
while True :
  c = s.read()
  for d in c:
     i = ord(d)
     rxBuffer[n]=i
     h = hex(i)[2:]
     o = o + ' ' + h
     n=n+1
     if n==5:
        (rxBuffer,n)=processFT817(rxBuffer,n,s)

exit()        
