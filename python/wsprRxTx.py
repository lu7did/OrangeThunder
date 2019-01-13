#!/usr/bin/python3
#*--------------------------------------------------------------------------
#* wsprRxTx
#*
#* Raspberry Pi based WSPR Rx/Tx configuration
#*
#* Send a WSPR frame and receive  WSPR the rest of the time
#* Based on the  packages
#*     WsprryPi (https://github.com/JamesP6000/WsprryPi)
#*     wsprd    (https://github.com/Guenael/rtlsdr-wsprd)
#* Plus some glueing Python code from my own cooking
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
#*
#* Version 1.1
#*    - improved subprocess management
#*    - telemetry
#*    - improved logging
#*    - changed from GPIO 17 to GPIO 27
#*    - better argument management
#*    - added checkpoint
#*    - added status
#*    - added running lock
#*    - Exception handling 
#* LU7DID
#*-------------------------------------------------------------------------
#* import the necessary packages
#*-------------------------------------------------------------------------
import argparse
import RPi.GPIO
import time
import subprocess
import os
import time
import sys
import datetime
import random
import RPi.GPIO as GPIO
import signal
#*--------------------------------------------------------------------------
#* WSPR Band Table
#*--------------------------------------------------------------------------
bands={
     "160m":1800000,
     "80m": 3500000,
     "40m": 7000000,
     "20m":14095600,
     "15m":21000000,
     "10m":28000000,
     "6m" :50000000,
     "2m":144000000
     }
#*-------------------------------------------------------------------------
#* Init global variables
#*-------------------------------------------------------------------------
id='LU7DID'
grid='GF05TE'
band='20m'
pwr='20'
freq=0
tx=True
VERSION='1.1'
PROGRAM=sys.argv[0].split(".")[0]
cycle=5
lckFile=("%s.lck") % PROGRAM
logFile=("%s.log") % PROGRAM
tlmFile=("%s.tlm") % PROGRAM
pidFile=("%s.pid") % PROGRAM
myPID=os.getpid()
DEBUGLEVEL=0
#*------------------------------------------------------------------------
#*  Set  GPIO out port for PTT
#*------------------------------------------------------------------------
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(27, GPIO.OUT)
#*-----------------------------------------------------------------------
#* manage locks
#*-----------------------------------------------------------------------
def createLock():
    log(0,"Archivo lock es %s" % lckFile)
    with open(lckFile, 'a') as f:
       f.write("%d\n" % myPID)

def resetLock():
    if isLock()==True:
       os.remove(lckFile)
    else:
       log(0,"The file %s does not exist" % lckFile)


def isLock():
    if os.path.exists(lckFile):
       return True
    else:
       return False 

def createPID():
    log(0,"Archivo PID es %s" % pidFile)
    with open(pidFile, 'a') as f:
       f.write("%d\n" % myPID)

def resetPID():
    if isPID()!= True:
       os.remove(pidFile)
    else:
       log(0,"The file %s does not exist" % pidFile)


def isPID():
    if os.path.exists(pidFile):
       return True
    else:
       return False
#*--------------------------------------------------------------------------
#* isRunning()
#*
#*--------------------------------------------------------------------------
def isRunning():
    if isPID() == True:
       f = open(pidFile, "r")
       for line in f:
           line=line.replace("\n","")
           log(1,"Returned line from pidFile %s" % line)
           return(line)
    else:
       log(0,"The % file does not exist" % pidFile)
       return ""


#*------------------------------------------------------------------------
#* setPTT
#* Activate the PTT thru GPIO27
#*------------------------------------------------------------------------
def setPTT(PTT):
    if PTT==False:
       GPIO.output(27, GPIO.LOW)
       log(0,"setPTT: GPIO.27=LOW PTT=%s Receiving" % PTT)
    else:
       GPIO.output(27, GPIO.HIGH)
       log(0,"setPTT: GPIO.27=HIGH PTT=%s Transmiting" % PTT)
      
#*-------------------------------------------------------------------------
#* getFreq(band)
#* Transform band into frequency
#*-------------------------------------------------------------------------
def getFreq(b):

    return bands.get(b,0)

#*-------------------------------------------------------------------------
#* log(string)
#* print log thru standard output
#*-------------------------------------------------------------------------
def log(d,st):
    if d<=DEBUGLEVEL:
       m=datetime.datetime.now()
       if logFile != '' : 
          with open(logFile, 'a') as f:
               f.write('%s %s\n' % (m,st))
       print('%s %s' % (m,st))
#*----------------------------------------------------------------------------
#* Exception and Termination handler
#*----------------------------------------------------------------------------
def signal_handler(sig, frame):
   log(0,"signal_handler: WSPR Monitor and Beacon is being terminated, clean up completed!")
   log(1,'Turning GPIO(27) low as PTT')
   setPTT(False)

   try:
     log(1,"Process terminated, clean up completed!")
     resetLock()
   except:
     log(1,"signal_handler: Process finalized")
   sys.exit(0)
#*-------------------------------------------------------------------------
#* Exception management
#*-------------------------------------------------------------------------
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

#*--------------------------------------------------------------------------
#* doExec()
#* Execute a command and return the result
#*--------------------------------------------------------------------------
def doExec(cmd):
    log(1,"doExec: [cmd] %s" % cmd)
    p = subprocess.Popen(cmd, shell=True, 
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    rc=p.wait()
    st=''
    for line in p.stdout:
        st=st+line.decode("utf-8").replace("\n","")
    return st

#*--------------------------------------------------------------------------
#* doShell()
#* Execute a command as a shell and print to std out and std error
#*--------------------------------------------------------------------------
def doShell(cmd):

    log(1,"doShell: [cmd] %s" % cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (result, error) = p.communicate()

    rc = p.wait()

    if rc != 0:
        log(0,"Error: failed to execute command: %s" % cmd)
        log(0,error)
    return result

#*--------------------------------------------------------------------------
#* doService()
#* Manages the monitor service
#*--------------------------------------------------------------------------
def doService(freq):

    log(0,'Time Executing time synchronization')
    cmd='sudo chronyc -a makestep'
    #cmd='sudo /home/pi/ntpd.sync'
    result=doShell(cmd)
    log(0,"doService: TimeSync(%s)" % result)
 
    while True:
       #*--------------------------*
       #* PTT low (receive)        *
       #*--------------------------*
       setPTT(False)

       #*--------------------------*
       #* Read and log telemetry   *
       #*--------------------------*
       cmd="python /home/pi/WsprryPi/picheck.py -a"
       result=doShell(cmd)
       log(0,"[TL] %s" % result.replace("\n",""))
       with open(tlmFile, 'a') as tlm:
          log(1,"writing telemetry to file %s" % tlmFile)
          tlm.write("%s" % result)

       #*--------------------------*
       #* Receive WSPR             *
       #*--------------------------*
       if cycle>1:
          n=getRandom(1,cycle)
       else:
          n=cycle
       cmd='sudo /home/pi/rtlsdr-wsprd/rtlsdr_wsprd -f %d -c %s -l %s -d 2 -n %d -a 1 -S' % (freq,id,grid,n)
       result=doShell(cmd)       
       log(0,"[RX]\n%s" % result)

       #*--------------------------*
       #* PTT high (transmit)      *
       #*--------------------------*
       setPTT(True)

       #*--------------------------*
       #* Transmit cycle           *
       #*--------------------------*
       cmd='sudo /home/pi/WsprryPi/wspr -r -o -s -x 1 %s %s %s %s' % (id,grid,pwr,band)
       result=doShell(cmd)       
       log(0,"[TX]\n%s" % result)

#*--------------------------------------------------------------------------
#* getRandom
#* Return an integer random number between two values [min,max]
#*--------------------------------------------------------------------------
def getRandom(nmin,nmax):
    return random.randint(nmin,nmax)

#*--------------------------------------------------------------------------
#* startService()
#*
#*--------------------------------------------------------------------------
def startService():
    if (id=='') or (grid=='') or (band==''):
       log(0,'Start command required id,grid and band to be set, exit')
       exit() 

    freq=getFreq(band)
    if freq==0 :
       log(0,'Non supported band(%s), exit' % band)
       exit()
    log(1,"Starting daemon PID(%d) and creating %s lock" % (myPID,lckFile))
    createLock()
    log(1,"%s lock file created" % lckFile)
    try:
      doService(freq)
    except:
      log(0,"Exception detected, program being terminated")
    else:
      log(0,"Program is ending normally")
#*============================================================================
#* Main Program
#*
#*============================================================================

#*---------------------------------------------------------------------------
# construct the argument parse and parse the arguments
#*---------------------------------------------------------------------------

ap = argparse.ArgumentParser()

#*--- Add all defined arguments

ap.add_argument("--start", help="Start the WSPR Transceiver", required=False, action="store_true")
ap.add_argument("--stop",help="Stop the WSPR Transceiver",required=False, action="store_true")
ap.add_argument("--list",help="List the WSPR Transceiver", required=False, action="store_true")
ap.add_argument("--id",help="Callsign to be used",required=False)
ap.add_argument("--grid",help="QTH Locator",required=False)
ap.add_argument("--band",help="Band to use i.e. 20m",required=False)
ap.add_argument("--pwr",help="Set Power in dBm",required=False)
ap.add_argument("--tx",help="Enable TX",required=False, action="store_true")
ap.add_argument("--cycle",help="Cycles RX/TX",required=False)
ap.add_argument("--log",help="Log activity to file",default=True,required=False,action="store_true")
ap.add_argument("--lock",help="Lock further execution till reset",required=False,action="store_true")
ap.add_argument("--reset",help="Reset locked execution", required=False,action="store_true")
args=ap.parse_args()

#*---------------------------*
#* Process log command       *
#*---------------------------*

if args.log == True :
   logFile="%s.log" % PROGRAM
   with open(logFile, 'w+') as f:
      f.write("")
   log(2,'Set logfile(%s)' % logFile)

log(0,"Program %s Version %s PID(%d)" % (PROGRAM,VERSION,os.getpid()))

#*---------------------------*
#* Process id command        *
#*---------------------------*

if args.id != None :
   id=args.id.upper()
   log(0,'Set Callsign id(%s)' % id)

#*---------------------------*
#* Process cycle command     *
#*---------------------------*

if args.cycle != None:
   cycle=int(args.cycle)
   log(0,'Set RX/TX cycle (%d)' % cycle)

#*---------------------------*
#* Process grid  command     *
#*---------------------------*

if args.grid != None :
   grid=args.grid.upper()
   log(0,'Set maiden QTH Locator grid(%s)' % grid)

#*---------------------------*
#* Process band command      *
#*---------------------------*

if args.band != None :
   band=args.band.upper()
   log(0,'Set Band(%s)' % band)

#*---------------------------*
#* Process Tx command        *
#*---------------------------*
if args.tx != False :
   tx=args.tx
   log(0,'Set Tx(%s)' % str(tx))

#*---------------------------*
#* Process Power command     *
#*---------------------------*
if args.pwr != None :
   pwr=args.pwr
   log(0,'Set Power(%s)' % pwr)

#*---------------------------*
#* Process start command     *
#*---------------------------*
if args.start:
       if isLock() == True:
          log(0,"Daemon is locked, run with --reset opetion to clear")
          exit()
       PID=isRunning()
       if PID == '':
          setPTT(False)
          startService()
       else:
          log(0,"Daemon already running PID(%s), exit" % PID)
       exit()
#*---------------------------*
#* Process stop command      *
#*---------------------------*
if args.stop:
       pid=isRunning()
       if(pid == ''):
         log(0,'Daemon is not running, exit')
       else:
         log(0,'Daemon found PID(%s), killing it' % pid)
         n=int(pid)
         cmd="sudo kill %s" % n
         result=doShell(cmd)
       setPTT(False)
       exit()

#*----------------------------------------------------------------------------
#* Review lock status
#*----------------------------------------------------------------------------
if args.lock == True:
       if isLock() == True:
          log(0,"Daemon is already locked, exit")
          exit()
       pid=isRunning()
       if(pid == ''):
         log(0,'Daemon is not running, locking and exit')
       else:
         log(0,'Daemon found PID(%s), killing, locking and exit' % pid)
         n=int(pid)
         cmd="sudo kill %s" % n
         result=doShell(cmd)
       setPTT(False)
       createLock()
       exit()

if args.reset == True:
       pid=isRunning()
       if(pid == ''):
         log(0,'Lock not found, exit')
       else:
         log(0,'Lock found PID(%s), killing it' % pid)
         n=int(pid)
       setPTT(False)
       resetLock()
       exit()


#*------------------------------------*
#* Process list command (default)     *
#*------------------------------------*
pid=isRunning()
if (pid == ''):
    print(0,'Status: Daemon is not running, exit')
else:
    print(0,'Status: Daemon is runnint PID(%s)' % pid)
exit() 
#*--------------------------------[End of program] -----------------------------------------------
