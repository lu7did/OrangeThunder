#!/bin/phyton
# -*- coding: latin-1 -*-
from __future__ import with_statement
#*--------------------------------------------------------------------------------
#* picheck.py
#* script to obtain physical variables of the host environment
#*--------------------------------------------------------------------------------
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
import subprocess
#*--------------------------------------------------------------------------------
#* Define Raspberry pi abnormal operation conditions
#*--------------------------------------------------------------------------------
pi_status={
    0 : "under-voltage",
    1 : "arm frequency capped",
    2 : "currently throttled", 
   16 : "under-voltage has occurred",
   17 : "arm frequency capped has occurred",
   18 : "throttling has occurred" }
#*--------------------------------------------------------------------------------
#* Extract a substring after a delimiter is found
#*--------------------------------------------------------------------------------
def substring_after(s, delim):
    return s.partition(delim)[2]
#*--------------------------------------------------------------------------------
#* Identify if the nth bit is set in the x word variable
#*--------------------------------------------------------------------------------
def is_set(x, n):
    return (x & 2 ** n != 0) 
#*--------------------------------------------------------------------------------
#* Execute a command and return the result
#*--------------------------------------------------------------------------------
def execute(cmd):
    """
        Purpose  : To execute a command and return exit status
        Argument : cmd - command to execute
        Return   : exit_code
    """
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (result, error) = process.communicate()

    rc = process.wait()

    if rc != 0:
        print "Error: failed to execute command:", cmd
        print error
    return result
#*----------------------------------------------------------------------------------
#* read_temperature
#* read the core temperature and report it
#*----------------------------------------------------------------------------------
def read_temperature ():
    cmd = "vcgencmd measure_temp"
    temp= execute(cmd)
    temp= temp.split('\'')[0]
    return substring_after(temp,"=")    
#*----------------------------------------------------------------------------------
#* read the arm core voltage and report it
#*----------------------------------------------------------------------------------
def read_voltage():
   cmd = "vcgencmd measure_volts"
   volt= execute(cmd)
   volt= volt.split('V')[0]
   return substring_after(volt,"=")
#*----------------------------------------------------------------------------------
#* read the arm core voltage and report it
#*----------------------------------------------------------------------------------
def read_frequency():
   cmd = "vcgencmd measure_clock arm"
   f= execute(cmd)
   #f= f.split('=')[0]
   return substring_after(f,"=")


#-----------------------------------------------------------------------------------
#* read the status word and parse results
#*----------------------------------------------------------------------------------
def read_status(p):

    if p!="":
       t=p
    else:
       cmd="vcgencmd get_throttled"
       t=execute(cmd)
       t=substring_after(t,"=")
    i=int(t,16)
    st=""
    for k, v in pi_status.items():
        if is_set(i,k) :
           st=st+("/%s" % (v))
    if st=="":
       return "Normal operation"
    else:
       return "Abnormal condition "+st
#*-------------------------------------------------------------------------------
#*                                       MAIN 
#*-------------------------------------------------------------------------------
#* Define argument structure
#*-------------------------------------------------------------------------------
p = argparse.ArgumentParser()
p.add_argument('-t', help="Get temperature (C)",action="store_true")
p.add_argument('-v', help="Get voltage",action="store_true")
p.add_argument('-f', help="Get frequency",action="store_true")
p.add_argument('-s', help="Get operational condition (get_throttled)",action="store_true")
p.add_argument('-x', help="Verbose output",action="store_true")
p.add_argument('-z', help="Special test status value 0xnn")
p.add_argument('-a', help="All telemetry in one call",action="store_true")


args = p.parse_args()

#*---------------------------------------------------------------------------------
#* Process arguments
#*---------------------------------------------------------------------------------
if args.a == True:
   st="Temp=%s °C Volt=%s V Clock(arm)=%s MHz Status(%s)" % (read_temperature(),read_voltage(),read_frequency().replace("\n",""),read_status(""))
   print(st)
   exit()

if args.t == True:
   if args.x == False:
      print(read_temperature())
   else:
      print("Temperature: %s C°" % read_temperature())
   exit()
if args.v == True:
   if args.x == False:
      print(read_voltage())
   else:
      print("Voltage: %s V" % read_voltage())
   exit()
if args.f == True:
   if args.x == False:
      print(read_frequency())
   else:
      print("Frequency (arm): %s MHz" % read_frequency())
   exit()
if args.s == True:
   if args.z != None:
      st=read_status(args.z)
   else:
      st=read_status("")
   if args.x == False:
      print(st)
   else:
      print("Status: %s" % st)
   exit()
  

#*------------------------------- End of Program --------------------------------------------
exit()





