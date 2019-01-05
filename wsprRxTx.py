
#!/usr/bin/python3
#*--------------------------------------------------------------------------
#* wsprRxTx
#*
#* Raspberry Pi based WSPR Rx/Tx configuration
#*
#* Send a WSPR frame and receive  WSPR the rest of the time
#* 
#* LU7DID
#* Acrux (192.168.0.200)
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
     "160M":1800000,
     "80M": 3500000,
     "40M": 7000000,
     "20M":14095600,
     "15M":21000000,
     "10M":28000000,
     "6M" :50000000,
     "2M":144000000
     }
#*-------------------------------------------------------------------------
#* Init global variables
#*-------------------------------------------------------------------------
id='LT7D'
grid='GF05TE'
band='20M'
pwr='10'
freq=0
tx=True
VERSION='1.0'
PROGRAM='wsprRxTx'
cycle=5
logFile='wsprRxTx.log'
myPID=os.getpid()
#*------------------------------------------------------------------------
#*  Set  GPIO out port for PTT
#*------------------------------------------------------------------------
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(17, GPIO.OUT)

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
def log(st):
    m=datetime.datetime.now()
    if logFile != '' :    
       print('%s %s' % (m,st),file=open(logFile, 'a'))
    else:
       print('%s %s' % (m,st))

#*-------------------------------------------------------------------------
#* isProcess(string)
#* Return PID of a process if exits 0 otherwise
#*
#*-------------------------------------------------------------------------
def isProcess(procName):
    cmd = 'sudo ps -aux | pgrep %s' % procName
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for line in p.stdout.readlines():
        st=line.decode("utf-8")
        if st=="":
           return 0
        i=int(st)
        if i==os.getpid():
           pass
        else:
           return i
    return 0
    retval = p.wait()
#*--------------------------------------------------------------------------
#* isFile(filename)
#* Check if a file exits
#*--------------------------------------------------------------------------
def isFile(filename):
  try:
    os.stat(filename)
    return True
  except:
    return False
#*--------------------------------------------------------------------------
#* doExec()
#* Execute a command and return the result
#*--------------------------------------------------------------------------
def doExec(cmd):
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
    log('Shell(%s)' % cmd)
    p = subprocess.Popen(cmd, shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)

    rc=p.wait()
    for line in p.stdout:
        log(line.decode("utf-8").replace("\n", ""))
    log('----------------[Shell End rc=%s]------------------------' % rc)


#*--------------------------------------------------------------------------
#* doService()
#* Manages the service
#*--------------------------------------------------------------------------
def doService(freq):

    log('Executing ntpd sync')
    cmd='sudo /home/pi/ntpd.sync'
    doShell(cmd)
 
    while True:
       #*--------------------------*
       #* PTT low (receive)        *
       #*--------------------------*
       log('Turning GPIO(17) low as PTT')
       GPIO.output(17, GPIO.LOW)

       #*--------------------------*
       #* Receive WSPR             *
       #*--------------------------*
       #if cycle>1:
       #   n=getRandom(1,cycle)
       #else:
       n=cycle
       cmd='sudo /home/pi/rtlsdr-wsprd/rtlsdr_wsprd -f %d -c %s -l %s -d 2 -n %d -a 1 -S' % (freq,id,grid,n)
       doShell(cmd)       

       #*--------------------------*
       #* PTT high (transmit)      *
       #*--------------------------*

       log('Turning GPIO(17) high as PTT')
       GPIO.output(17, GPIO.HIGH)

       #*--------------------------*
       #* Transmit cycle           *
       #*--------------------------*
       cmd='sudo /home/pi/WsprryPi/wspr -r -o -s -x 1 %s %s %s %s' % (id,grid,pwr,band)
       doShell(cmd)       


#*-------------------------------------------------------------------------
#*  killService()
#*  Kill a given process by Id
#*-------------------------------------------------------------------------
def killService(pid):
    cmd='sudo kill %d' % pid   
    doShell(cmd)

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
       log('Start command required id,grid and band to be set, exit')
       exit() 
    freq=getFreq(band)
    if freq==0 :
       log('Non supported band(%s), exit' % band)
       exit()
    log("Starting daemon PID(%d) and creating /var/run/wsprRxTx/wsprRxTx.pid" % myPID)
    os.system ("sudo mkdir /var/run/wsprRxTx")
    os.system ("sudo echo %d > /var/run/wsprRxTx/wsprRxTx.pid" % myPID)
    try:
      doService(freq)
    except:
      log("Exception detected, program being terminated")
    else:
      log("Program is ending normally")
#*--------------------------------------------------------------------------
#* isRunning()
#*
#*--------------------------------------------------------------------------
def isRunning():
    return doExec('cat /var/run/wsprRxTx/wsprRxTx.pid')
#*--------------------------------------------------------------------------
#* getDate()
#*
#*--------------------------------------------------------------------------
def getDate():
   hour = time.strftime("%H")
   min=time.strftime("%M")
   sec=time.strftime("%S")
   print ("la hora STRING es %s %s %s" % (hour,min,sec))

   h=int(hour)
   m=int(min)
   s=int(sec)

   print("la hora numerica es %d %d %d" % (h,m,s))
   time.sleep(1)

#*----------------------------------------------------------------------------
#* Exception and Termination handler
#*----------------------------------------------------------------------------
def signal_handler(sig, frame):
   log("Process terminated, clean up completed!")
   os.system ("sudo rm -r /var/run/wsprRxTx/wsprRxTx.pid")
   os.system ("sudo rm -r /var/run/wsprRxTx")
   sys.exit(0)
#*-------------------------------------------------------------------------
#* Exception management
#*-------------------------------------------------------------------------
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

#*============================================================================
#* Main Program
#*
#*============================================================================
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

args=ap.parse_args()

#*---------------------------*
#* Process log command       *
#*---------------------------*

if args.log == True :
   logFile="%s.log" % PROGRAM
   print(' ',file=open(logFile, 'w+'))
   log('Set logfile(%s)' % logFile)

log("Program %s Version %s PID(%d)" % (PROGRAM,VERSION,os.getpid()))

#*---------------------------*
#* Process id command        *
#*---------------------------*

if args.id != None :
   id=args.id.upper()
   log('Set Callsign id(%s)' % id)

#*---------------------------*
#* Process cycle command     *
#*---------------------------*

if args.cycle != None:
   cycle=int(args.cycle)
   log('Set RX/TX cycle (%d)' % cycle)

#*---------------------------*
#* Process grid  command     *
#*---------------------------*

if args.grid != None :
   grid=args.grid.upper()
   log('Set maiden QTH Locator grid(%s)' % grid)

#*---------------------------*
#* Process band command      *
#*---------------------------*

if args.band != None :
   band=args.band.upper()
   log('Set Band(%s)' % band)
#*---------------------------*
#* Process Tx command        *
#*---------------------------*
if args.tx != False :
   tx=args.tx
   log('Set Tx(%s)' % str(tx))

#*---------------------------*
#* Process Power command     *
#*---------------------------*
if args.pwr != None :
   pwr=args.pwr
   log('Set Power(%s)' % pwr)

#*---------------------------*
#* Process start command     *
#*---------------------------*
if args.start:
       if isRunning() == '':
          startService()
       else:
          log("Daemon already running, exit")
       exit()
#*---------------------------*
#* Process stop command      *
#*---------------------------*
if args.stop:
       pid=isRunning()
       if(pid == ''):
         log('Daemon is not running, exit')
         exit()
       else:
         log('Daemon found PID(%s), killing it' % pid)
         n=int(pid)
         killService(n)
         exit()
#*---------------------------*
#* Process list command      *
#*---------------------------*
if args.list:
      pid=isRunning()
      if (pid == ''):
         print('Daemon is not running, exit')
         exit()
      else:
         print('Daemon found PID(%s)' % pid)
         exit() 
#*--------------------------------[End of program] -----------------------------------------------
