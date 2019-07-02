import sys
import csv
import time
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.basemap import Basemap
import datetime


def GetLon(ONE, THREE, FIVE):
    StrStartLon = ''
    StrEndLon = ''

    Field = ((ord(ONE.lower()) - 97) * 20) 
    Square = int(THREE) * 2
    SubSquareLow = (ord(FIVE.lower()) - 97) * (2/24)
    SubSquareHigh = SubSquareLow + (2/24)

    StrStartLon = str(Field + Square + SubSquareLow - 180 )
    StrEndLon = str(Field + Square + SubSquareHigh - 180 )

    return StrStartLon, StrEndLon

def GetLat(TWO, FOUR, SIX):
    StrStartLat = ''
    StrEndLat = ''

    Field = ((ord(TWO.lower()) - 97) * 10) 
    Square = int(FOUR)
    SubSquareLow = (ord(SIX.lower()) - 97) * (1/24)
    SubSquareHigh = SubSquareLow + (1/24)

    StrStartLat = str(Field + Square + SubSquareLow - 90)
    StrEndLat = str(Field + Square + SubSquareHigh - 90)    

    return StrStartLat, StrEndLat

def GridToLatLong(strMaidenHead):
    if len(strMaidenHead) < 6: strMaidenHead=strMaidenHead+"aa" 

    ONE = strMaidenHead[0:1]
    TWO = strMaidenHead[1:2]
    THREE = strMaidenHead[2:3]
    FOUR = strMaidenHead[3:4]
    FIVE = strMaidenHead[4:5]
    SIX = strMaidenHead[5:6]

    (strStartLon, strEndLon) = GetLon(ONE, THREE, FIVE)
    (strStartLat, strEndLat) = GetLat(TWO, FOUR, SIX)

    #print ('Start Lon = ' + strStartLon)
    #print ('End   Lon = ' + strEndLon)
    #print ()
    #print ('Start Lat = ' + strStartLat)
    #print ('End   Lat = ' + strEndLat)

    return strStartLon,strStartLat

def plotMap(map,gFrom,gTo):

    lonFrom,latFrom=GridToLatLong(gFrom.strip())
    lonTo,latTo=GridToLatLong(gTo.strip())


    loFrom=float(lonFrom)
    loTo=float(lonTo)
    laFrom=float(latFrom)
    laTo=float(latTo)


    lat = [laFrom,laTo] 
    lon = [loFrom,loTo] 


    x, y = map(lon, lat)
    map.plot(x, y, 'o-', markersize=1, linewidth=1) 
    return

def buildMap():
    m = Basemap(width=1024,height=1024,projection='merc',llcrnrlon=-170,llcrnrlat=-75,urcrnrlon=170,urcrnrlat=75,resolution='l')


    m.drawmeridians(np.arange(0,360,30))
    m.drawparallels(np.arange(-90,90,30))
    m.drawcoastlines(linewidth=0.25)
    m.drawcountries(linewidth=0.25)
    return m




lastHour=0
c=0
MH= 'GF05te'

#map = Basemap(projection='ortho',lat_0=-34.6,lon_0=-58.4,resolution='c')
#*---------------------------------------------------------------------------------------------------
# Process WSPRNet dataset with awk '{print "plotMap(map,\""$7"\",\""$10"\")"}' wsprdata.lst > set.py
#*---------------------------------------------------------------------------------------------------

hour=0

f = datetime.datetime.now()
x = datetime.datetime.utcnow()

print("Initialization of maps LOCAL  %s\n" % (f.strftime("%b %d %Y %H:%M:%S")))
print("Initialization of maps UTC    %s\n" % (x.strftime("%b %d %Y %H:%M:%S")))

map=buildMap();

#*-------------------------------------------------------------------------------------
#* Scan data and build datasets
#*-------------------------------------------------------------------------------------
for row in csv.reader(iter(sys.stdin.readline, ''),delimiter='\t'):

#*--- Parse data out of the dataset

    timestamp=row[0]
    toCall=row[1]
    freq=row[2]
    SNR=row[3]
    DT=row[4]
    toLocator=row[5]
    pwr=row[6]
    fromCall=row[7]
    fromLocator=row[8]
    hour=int(timestamp.split(':')[0])
    band=freq.split('.')[0]

    #print("Hour(%s) Time(%s) toCall(%s) freq(%s) SNR(%s) DT(%s) locatorTo(%s) pwr(%s) fromCall(%s) locatorFrom(%s)" % (hour,timestamp,toCall,freq,SNR,DT,toLocator,pwr,fromCall,fromLocator))

#*--- Identify change of the hour

    while hour!=lastHour:


      x = datetime.datetime.utcnow()
      f = datetime.datetime(x.year,x.month,x.day,lastHour,0,0)
      print("\nBand %sMHz Processing spots for hour %d Spots(%d)\n " % (band,lastHour,c))

      CS=map.nightshade(f)
      #map.shadedrelief(scale=0.25)
      map.bluemarble(scale=0.25)

      plt.title("Band %s Hour GMT %d\n" % (band,lastHour))
      if len(str(lastHour)) == 1:
         stHour="0"+str(lastHour)
      else:
         stHour=str(lastHour)
      plt.savefig("condx"+stHour+".png")
     #plt.savefig('condx.pdf')
     #plt.show()
      plt.close("all")
      print("Image generation for hour hour %s has been completed\n" % lastHour)
      map=None
      map=buildMap()

      lastHour=lastHour+1
      c=0
      if lastHour>24:
          print("EoF\n")
          exit(0)
    if hour==lastHour:
       #print("Band %s processing hour %d\n" % (band,lastHour))
       c=c+1
       plotMap(map,toLocator,fromLocator)

while lastHour<24:
   map=None
   map=buildMap()
   x = datetime.datetime.utcnow()
   f = datetime.datetime(x.year,x.month,x.day,lastHour,0,0)
   print("\nBand %sMHz Processing spots for hour %d Spots(%d)\n " % (band,lastHour,c))

   CS=map.nightshade(f)
   #map.shadedrelief(scale=0.25)
   map.bluemarble(scale=0.25)

   plt.title("Band %s Hour GMT %d\n" % (band,lastHour))
   if len(str(lastHour)) == 1:
      stHour="0"+str(lastHour)
   else:
      stHour=str(lastHour)
   plt.savefig("condx"+stHour+".png")
  #plt.savefig('condx.pdf')
  #plt.show()
   plt.close("all")
   print("Image generation for hour hour %s has been completed\n" % lastHour)
   lastHour=lastHour+1
