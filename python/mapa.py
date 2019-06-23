import matplotlib.pyplot as plt
import numpy as np

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
    if len(strMaidenHead) < 6: strMaidenHead = __MH__

    ONE = strMaidenHead[0:1]
    TWO = strMaidenHead[1:2]
    THREE = strMaidenHead[2:3]
    FOUR = strMaidenHead[3:4]
    FIVE = strMaidenHead[4:5]
    SIX = strMaidenHead[5:6]

    (strStartLon, strEndLon) = GetLon(ONE, THREE, FIVE)
    (strStartLat, strEndLat) = GetLat(TWO, FOUR, SIX)

    print ('Start Lon = ' + strStartLon)
    print ('End   Lon = ' + strEndLon)
    print ()
    print ('Start Lat = ' + strStartLat)
    print ('End   Lat = ' + strEndLat)

    return strStartLon,strStartLat

MH= 'GF05te'
Lon,Lat=GridToLatLong(MH)
print("Convertir %s --> Lon=%s Lat=%s" % (MH,Lon,Lat)) 

#map = Basemap(projection='ortho',lat_0=-34.6,lon_0=-58.4,resolution='c')
from mpl_toolkits.basemap import Basemap

map = Basemap(width=1024,height=1024,projection='merc',llcrnrlon=-170,llcrnrlat=-75,urcrnrlon=170,urcrnrlat=75,resolution='l')


map.drawmeridians(np.arange(0,360,30))
map.drawparallels(np.arange(-90,90,30))
map.drawcoastlines(linewidth=0.25)
map.drawcountries(linewidth=0.25)





lat = [-34.6,48.86] 
lon = [-58.4,2.34] 


x, y = map(lon, lat)
map.plot(x, y, 'o-', markersize=1, linewidth=1) 
map.bluemarble(scale=0.25)



#map.shadedrelief()


# draw the edge of the map projection region (the projection limb)
#map.drawmapboundary(fill_color='aqua')
# draw lat/lon grid lines every 30 degrees.
# make up some data on a regular lat/lon grid.
#nlats = 73; nlons = 145; delta = 2.*np.pi/(nlons-1)
#lats = (0.5*np.pi-delta*np.indices((nlats,nlons))[0,:,:])
#lons = (delta*np.indices((nlats,nlons))[1,:,:])
#wave = 0.75*(np.sin(2.*lats)**8*np.cos(4.*lons))
#mean = 0.5*np.cos(2.*lats)*((np.sin(2.*lats))**2 + 2.)
# compute native map projection coordinates of lat/lon grid.
#x, y = map(lons*180./np.pi, lats*180./np.pi)
# contour data over the map.
#cs = map.contour(x,y,wave+mean,15,linewidths=1.5)
plt.title('Propagation Forecast Image')
plt.savefig('condx.png')
plt.savefig('condx.pdf')
plt.show()

exit(0)
# set up orthographic map projection with
# perspective of satellite looking down at 50N, 100W.
# use low resolution coastlines.
#width = 28000000; lon_0 = -105; lat_0 = 40
#map = Basemap(width=width,height=width,projection='aeqd',lat_0=lat_0,lon_0=lon_0)


# llcrnrlat,llcrnrlon,urcrnrlat,urcrnrlon
# are the lat/lon values of the lower left and upper right corners
# of the map.
# lat_ts is the latitude of true scale.
# resolution = 'c' means use crude resolution coastlines.
#map = Basemap(projection='merc',llcrnrlat=-80,urcrnrlat=80,llcrnrlon=-180,urcrnrlon=180,lat_ts=20,resolution='c')
#map.drawcoastlines()
#map.fillcontinents(color='coral',lake_color='aqua')



# draw coastlines, country boundaries, fill continents.
#map.drawcoastlines(linewidth=0.25)
#map.drawcountries(linewidth=0.25)
#map.fillcontinents(color='coral',lake_color='aqua')

#map = Basemap(width=12000000,height=9000000,projection='lcc',resolution=None,lat_1=45.,lat_2=55,lat_0=50,lon_0=-58.4)
#map.bluemarble()

#map = Basemap(projection='merc',llcrnrlat=-89,urcrnrlat=89,resolution='l')
