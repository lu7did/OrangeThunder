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
PROGRAM=sys.argv[0]
cycle=5
lckFile=("%s.lck") % PROGRAM.split(".")[0]
logFile=("%s.log") % PROGRAM.split(".")[0]
tlmFile=("%s.tlm") % PROGRAM.split(".")[0]
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
    if os.path.exists(lckFile):
       os.remove(lckFile)
    else:
       log(0,"The file %s does not exist" % lckFile)
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
    m=datetime.datetime.now()
    if logFile != '' : 
       with open(logFile, 'a') as f:
         #f.write(text)   
         f.write('%s %s\n' % (m,st))
       print('%s %s' % (m,st))
#*----------------------------------------------------------------------------
#* Exception and Termination handler
#*----------------------------------------------------------------------------
def signal_handler(sig, frame):
   log(0,"signal_handler: WSPR Monitor and Beacon is being terminated, clean up completed!")
   log(0,'Turning GPIO(27) low as PTT')
   #GPIO.output(27, GPIO.LOW)
   setPTT(False)

   try:
     log(0,"Process terminated, clean up completed!")
     resetLock()
   except:
     log(0,"signal_handler: Process finalized")
   sys.exit(0)
#*-------------------------------------------------------------------------
#* Exception management
#*-------------------------------------------------------------------------
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

#*----------------------------------------------------------------------------
#* Exception and Termination handler
#*----------------------------------------------------------------------------
#def killProc(p):#
#
#   try:
#     parent = psutil.Process(p.pid)
#     log(2,"killProc: process %s)" % parent)
#     for child in parent.children(recursive=True):  # or parent.children() for recursive=False
#       log(2,"killProc:     -- child(%s)" % child)
#       child.kill()
#     parent.kill()
#   except UnboundLocalError as error:
#     log(0,"killProc: Unable to kill parent PID")
#   except NameError as error:
#     log(2,"killProc: Allocation error")
#   except Exception as exception:
#     pass
#*-----------------------------------------------------------------------------------------------
#* waitProc
#* waiting for a process to start looking for a given string
#*-----------------------------------------------------------------------------------------------
#def waitProc(p,stOK,timeout):
#    log(2,"waitProc: waiting for string(%s) for timeout(%d)" % (stOK,timeout))
#    ts=time.time()
#    while (time.time()-ts) <= timeout:
#        s=non_block_read(p.stdout)
#        if s!="":
#           if s.find(stOK) != -1:
#              log(2,"waitProc: process (%s) found OK string (%s)" % (str(psutil.Process(p.pid)),stOK))
#              return 0
#           else:
#              log(1,s.replace("\n",""))
#    return -1
#*------------------------------------------------------------------------------------------------
#def startProc(cmd,stOK):
#    log(2,"startProc: cmd(%s) OK(%s)" % (cmd,stOK))
#    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#    if waitProc(p,stOK,5) == 0:
#       log(1,"startProc: Process successfully launched")
#       return p
#    log(0,"startProc: failed to launch process *ABORT*")
#    raise Exception('startProc: general exceptions not caught by specific handling')
#    return None
#*-------------------------------------------------------------------------
#* isProcess(string)
#* Return PID of a process if exits 0 otherwise
#*
#*-------------------------------------------------------------------------
#def isProcess(procName):
#    cmd = 'sudo ps -aux | pgrep %s' % procName
#    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#    for line in p.stdout.readlines():
#        st=line.decode("utf-8")
#        if st=="":
#           return 0
#        i=int(st)
#        if i==os.getpid():
#           pass
#        else:
#           return i
#    return 0
#    retval = p.wait()
#*--------------------------------------------------------------------------
#* isFile(filename)
#* Check if a file exits
#*--------------------------------------------------------------------------
#def isFile(filename):
#  try:
#    os.stat(filename)
#    return True
#  except:
#    return False
#
#*--------------------------------------------------------------------------
#* doExec()
#* Execute a command and return the result
#*--------------------------------------------------------------------------
def doExec(cmd):
    log(0,"doExec: [cmd] %s" % cmd)
    p = subprocess.Popen(cmd, shell=True, 
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    rc=p.wait()
    st=''
    for line in p.stdout:
        st=st+line.decode("utf-8").replace("\n","")
    #log(0,"doExec: [result] %s" % st)
    #log(0,"doExec: [error] %s" % p.stderr)
    return st

#*--------------------------------------------------------------------------
#* doShell()
#* Execute a command as a shell and print to std out and std error
#*--------------------------------------------------------------------------
def doShell(cmd):

    log(0,"doShell: [cmd] %s" % cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (result, error) = p.communicate()

    rc = p.wait()

    if rc != 0:
        log(0,"Error: failed to execute command: %s" % cmd)
        log(0,error)
    #log(0,"doShell: [Result] %s" %  result)
    return result

#*--------------------------------------------------------------------------
#* doService()
#* Manages the monitor service
#*--------------------------------------------------------------------------
def doService(freq):

    log(0,'Executing time synchronization (ntpd sync)')
    cmd='sudo /home/pi/ntpd.sync'
    result=doShell(cmd)
    log(0,"doService: ntpd(%s)" % result)
 
    while True:
       #*--------------------------*
       #* PTT low (receive)        *
       #*--------------------------*
       #GPIO.output(27, GPIO.LOW)
       setPTT(False)

       #*--------------------------*
       #* Read and log telemetry   *
       #*--------------------------*
       cmd="python /home/pi/WsprryPi/picheck.py -a"
       result=doShell(cmd)
       log(0,"[TL] %s" % result.replace("\n",""))
       with open(tlmFile, 'a') as tlm:
          log(0,"writing telemetry to file %s" % tlmFile)
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
    log(0,"Starting daemon PID(%d) and creating %s lock" % (myPID,lckFile))
    createLock()
    log(0,"%s lock file created" % lckFile)
    try:
      doService(freq)
    except:
      log(0,"Exception detected, program being terminated")
    else:
      log(0,"Program is ending normally")
#*--------------------------------------------------------------------------
#* isRunning()
#*
#*--------------------------------------------------------------------------
def isRunning():
    if os.path.exists(lckFile):
       f = open(lckFile, "r")
       for line in f:
           line=line.replace("\n","")
           log(0,"Returned line from lckFile %s" % line)
           return(line)
    else:
       log(0,"The lockfile file does not exist")
       return ""

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
ap.add_argument("--log",help="Log activity to file",required=False,action="store_true")
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
   log(0,'Set logfile(%s)' % logFile)

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
       PID=isRunning()
       if PID == '':
          setPTT(False)
          startService()
       else:
          log(0,"Daemon already running PID(%s) or locked, exit" % PID)
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


if args.lock == True:
       pid=isRunning()
       if(pid == ''):
         log(0,'Daemon is not running, locking and exit')
       else:
         log(0,'Daemon found PID(%s), killing it' % pid)
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
    print(0,'Daemon is not running, exit')
else:
    print(0,'Daemon found PID(%s)' % pid)
exit() 
#*--------------------------------[End of program] -----------------------------------------------
