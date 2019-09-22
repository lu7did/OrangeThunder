#!/usr/bin/env python
from __future__ import division
import requests
import ephem
from datetime import datetime, timedelta
from satellite_tle import fetch_tles
import os
import lxml.html
import argparse
import logging
from utils import get_active_transmitter_info, get_transmitter_stats, \
    get_groundstation_info, get_scheduled_passes_from_network, ordered_scheduler, \
    report_efficiency, find_passes, schedule_observation, read_priorities_transmitters, \
    get_satellite_info, update_needed, get_priority_passes
import settings
from tqdm import tqdm
import sys

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']


class twolineelement:
    """TLE class"""

    def __init__(self, tle0, tle1, tle2):
        """Define a TLE"""

        self.tle0 = tle0
        self.tle1 = tle1
        self.tle2 = tle2
        if tle0[:2] == "0 ":
            self.name = tle0[2:]
        else:
            self.name = tle0
            if tle1.split(" ")[1] == "":
                self.id = int(tle1.split(" ")[2][:4])
            else:
                self.id = int(tle1.split(" ")[1][:5])


class satellite:
    """Satellite class"""

    def __init__(self, tle, transmitter, success_rate, good_count, data_count, mode):
        """Define a satellite"""

        self.tle0 = tle.tle0
        self.tle1 = tle.tle1
        self.tle2 = tle.tle2
        self.id = tle.id
        self.name = tle.name.strip()
        self.transmitter = transmitter
        self.success_rate = success_rate
        self.good_count = good_count
        self.data_count = data_count
        self.mode = mode
        
    def __repr__(self):
        return "%s %s %d %d %d %s %s" % (self.id, self.transmitter, self.success_rate, self.good_count,
                                         self.data_count, self.mode, self.name)


def _log_level_string_to_int(log_level_string):
    if log_level_string not in _LOG_LEVEL_STRINGS:
        message = 'invalid choice: {0} (choose from {1})'.format(log_level_string,
                                                                 _LOG_LEVEL_STRINGS)
        raise argparse.ArgumentTypeError(message)

    log_level_int = getattr(logging, log_level_string, logging.INFO)
    # check the logging log_level_choices have not changed from our expected values
    assert isinstance(log_level_int, int)

    return log_level_int

#*-----------------------------------------------------------------------------------
#* checkAz
#* Function to compute if Az is within a given Azimuth window
#*-----------------------------------------------------------------------------------
def checkAz(Az,Azmin,Azmax):

    if Az == 0:
       return True

    if Azmin>Azmax:
       if(Az>=Azmin) or (Az<=Azmax):
         return True
       else:
         return False
    else:
       if (Az>=Azmin) and (Az<=Azmax):
          return True
       else:
          return False
#*-------------------------------------------------------------------------------
def main():

    # Parse arguments
    parser = argparse.ArgumentParser(
        description="Automatically schedule observations on a SatNOGS station.")
    parser.add_argument("-s", "--station", help="Ground station ID", type=int)
    parser.add_argument("-t",
                        "--starttime",
                        help="Start time (YYYY-MM-DD HH:MM:SS) [default: now]",
                        default=datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S"))
    parser.add_argument("-d",
                        "--duration",
                        help="Duration to schedule [hours; default: 1.0]",
                        type=float,
                        default=1.0)
    parser.add_argument("-m",
                        "--min-culmination",
                        help="Minimum culmination elevation [degrees; ground station default, minimum: 0, maximum: 90]",
                        type=float,
                        default=None)
    parser.add_argument("-r",
                        "--min-riseset",
                        help="Minimum rise/set elevation [degrees; ground station default, minimum: 0, maximum: 90]",
                        type=float,
                        default=None)
#*-------------------------------------------------------------------------------------------------
#* define two new arguments to specify Az (min,max) window
#*-------------------------------------------------------------------------------------------------
    parser.add_argument("-a",
                        "--azmin",
                        help="Minimum Az window [degrees; ground station default=0,maximum: 359]",
                        type=float,
                        default=None)
    parser.add_argument("-A",
                        "--azmax",
                        help="Maximum Az window [degrees; ground station default=0,maximum: 359]",
                        type=float,
                        default=None)
#*--------------------------------------------------------------------------------------------------
    parser.add_argument("-z",
                        "--horizon",
                        help="Force rise/set elevation to 0 degrees (overrided -r).",
                        action="store_true")
    parser.add_argument("-f",
                        "--only-priority",
                        help="Schedule only priority satellites (from -P file)",
                        dest='only_priority',
                        action='store_false')
    parser.set_defaults(only_priority=True)
    parser.add_argument("-w",
                        "--wait",
                        help="Wait time between consecutive observations (for setup and slewing)" +
                        " [seconds; default: 0, maximum: 3600]",
                        type=int,
                        default=0)
    parser.add_argument("-n",
                        "--dryrun",
                        help="Dry run (do not schedule passes)",
                        action="store_true")
    parser.add_argument("-P",
                        "--priorities",
                        help="File with transmitter priorities. Should have " +
                        "columns of the form |NORAD priority UUID| like |43017 0.9" +
                        " KgazZMKEa74VnquqXLwAvD|. Priority is fractional, one transmitter " +
                        "per line.",
                        default=None)
    parser.add_argument("-M",
                        "--min-priority",
                        help="Minimum priority. Only schedule passes with a priority higher" +
                        "than this limit [default: 0.0, maximum: 1.0]",
                        type=float,
                        default=0.)
    parser.add_argument("-T",
                        "--allow-testing",
                        help="Allow scheduling on stations which are in testing mode [default: False]",
                        action="store_true")
    parser.set_defaults(allow_testing=False)    
    parser.add_argument("-l",
                        "--log-level",
                        default="INFO",
                        dest="log_level",
                        type=_log_level_string_to_int,
                        nargs="?",
                        help="Set the logging output level. {0}".format(_LOG_LEVEL_STRINGS))
    args = parser.parse_args()

    # Check arguments
    if args.station is None:
        parser.print_help()
        sys.exit()

    # Setting logging level
    numeric_level = args.log_level
    if not isinstance(numeric_level, int):
        raise ValueError("Invalid log level")
    logging.basicConfig(level=numeric_level,
                        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")

    # Settings

    ground_station_id = args.station
    if args.duration > 0.0:
        length_hours = args.duration
    else:
        length_hours = 1.0
    if args.wait <= 0:
        wait_time_seconds = 0
    elif args.wait <= 3600:
        wait_time_seconds = args.wait
    else:
        wait_time_seconds = 3600
    if args.min_priority < 0.0:
        min_priority = 0.0
    elif args.min_priority > 1.0:
        min_priority = 1.0
    else:
        min_priority = args.min_priority

    if (args.azmin >= 0) and (args.azmin <= 360):
       azmin=args.azmin
    else:
       azmin=0

    if (args.azmax>=0) and (args.azmax<=360):
       azmax=args.azmax
    else:
       azmax=365


    cache_dir = "/tmp/cache"
    schedule = not args.dryrun
    only_priority = args.only_priority
    priority_filename = args.priorities

    # Set time range
    tnow = datetime.strptime(args.starttime, "%Y-%m-%dT%H:%M:%S")
    tmin = tnow
    tmax = tnow + timedelta(hours=length_hours)

    # Get ground station information
    ground_station = get_groundstation_info(ground_station_id, args.allow_testing)
    if not ground_station:
        sys.exit()
    
    # Create cache
    if not os.path.isdir(cache_dir):
        os.mkdir(cache_dir)

    # Printing Az window
    logging.info('Az window set as Min(%3.0f) Max(%3.0f)' % (azmin,azmax))

    # Update logic
    update = update_needed(tnow, ground_station_id, cache_dir)

    # Update
    if update:
        logging.info('Updating transmitters and TLEs for station')
        # Store current time
        with open(os.path.join(cache_dir, "last_update_%d.txt" % ground_station_id), "w") as fp:
            fp.write(tnow.strftime("%Y-%m-%dT%H:%M:%S") + "\n")

        # Get active transmitters in frequency range of each antenna
        transmitters = {}
        for antenna in ground_station['antenna']:
            for transmitter in get_active_transmitter_info(antenna["frequency"],
                                                           antenna["frequency_max"]):
                transmitters[transmitter['uuid']] = transmitter

        # Get satellites which are alive
        alive_norad_cat_ids = get_satellite_info()

        # Get NORAD IDs
        norad_cat_ids = sorted(
            set([
                transmitter["norad_cat_id"] for transmitter in transmitters.values()
                if transmitter["norad_cat_id"] < settings.MAX_NORAD_CAT_ID and
                transmitter["norad_cat_id"] in alive_norad_cat_ids
            ]))

        # Store transmitters
        fp = open(os.path.join(cache_dir, "transmitters_%d.txt" % ground_station_id), "w")
        logging.info("Requesting transmitter success rates.")
        transmitters_stats = get_transmitter_stats()
        for transmitter in transmitters_stats:
            uuid = transmitter["uuid"]
            # Skip absent transmitters
            if uuid not in transmitters.keys():
                continue
            # Skip dead satellites
            if transmitters[uuid]["norad_cat_id"] not in alive_norad_cat_ids:
                continue

            fp.write(
                "%05d %s %d %d %d %s\n" %
                (transmitters[uuid]["norad_cat_id"], uuid, transmitter["stats"]["success_rate"],
                 transmitter["stats"]["good_count"], transmitter["stats"]["total_count"], transmitters[uuid]["mode"]))

        logging.info("Transmitter success rates received!")
        fp.close()

        # Get TLEs
        tles = fetch_tles(norad_cat_ids)

        # Store TLEs
        fp = open(os.path.join(cache_dir, "tles_%d.txt" % ground_station_id), "w")
        for norad_cat_id, (source, tle) in tles.items():
            fp.write("%s\n%s\n%s\n" % (tle[0], tle[1], tle[2]))
        fp.close()

    # Set observer
    observer = ephem.Observer()
    observer.lon = str(ground_station['lng'])
    observer.lat = str(ground_station['lat'])
    observer.elevation = ground_station['altitude']

    # Set minimum culmination elevation
    if args.min_culmination is None:
        min_culmination = ground_station['min_horizon']
    else:
        if args.min_culmination < 0.0:
            min_culmination = 0.0
        elif args.min_culmination > 90.0:
            min_culmination = 90.0
        else:
            min_culmination = args.min_culmination

    # Set minimum rise/set elevation
    if args.min_riseset is None:
        min_riseset = ground_station['min_horizon']
    else:
        if args.min_riseset < 0.0:
            min_riseset = 0.0
        elif args.min_riseset > 90.0:
            min_riseset = 90.0
        else:
            min_riseset = args.min_riseset
            
    # Use minimum altitude for computing rise and set times (horizon to horizon otherwise)
    if not args.horizon:
        observer.horizon = str(min_riseset)

    # Minimum duration of a pass
    min_pass_duration = settings.MIN_PASS_DURATION

    # Read tles
    with open(os.path.join(cache_dir, "tles_%d.txt" % ground_station_id), "r") as f:
        lines = f.readlines()
        tles = [
            twolineelement(lines[i], lines[i + 1], lines[i + 2]) for i in range(0, len(lines), 3)
        ]

    # Read transmitters
    satellites = []
    with open(os.path.join(cache_dir, "transmitters_%d.txt" % ground_station_id), "r") as f:
        lines = f.readlines()
        for line in lines:
            item = line.split()
            norad_cat_id, uuid, success_rate, good_count, data_count, mode = int(
                item[0]), item[1], float(item[2]) / 100.0, int(item[3]), int(item[4]), item[5]
            for tle in tles:
                if tle.id == norad_cat_id:
                    satellites.append(satellite(tle, uuid, success_rate, good_count, data_count, mode))

    # Find passes
    passes = find_passes(satellites, observer, tmin, tmax, min_culmination, min_pass_duration)

    priorities, favorite_transmitters = read_priorities_transmitters(priority_filename)
    
    # List of scheduled passes
    scheduledpasses = get_scheduled_passes_from_network(ground_station_id, tmin, tmax)
    logging.info("Found %d scheduled passes between %s and %s on ground station %d" %
                 (len(scheduledpasses), tmin, tmax, ground_station_id))

    # Get passes of priority objects
    prioritypasses, normalpasses = get_priority_passes(passes, priorities, favorite_transmitters,
                                                       only_priority, min_priority)

    # Priority scheduler
    prioritypasses = sorted(prioritypasses, key=lambda satpass: -satpass['priority'])
    scheduledpasses = ordered_scheduler(prioritypasses, scheduledpasses, wait_time_seconds)

    for satpass in passes:
        logging.debug(satpass)

    # Normal scheduler
    normalpasses = sorted(normalpasses, key=lambda satpass: -satpass['priority'])
    scheduledpasses = ordered_scheduler(normalpasses, scheduledpasses, wait_time_seconds)

    # Report scheduling efficiency
    report_efficiency(scheduledpasses, passes)

    # Find unique objects
    satids = sorted(set([satpass['id'] for satpass in passes]))

    schedule_needed = False

#*----- Modify to add AzR and AzS

    logging.info("GS  | Sch | NORAD | Start time          | End time            |  El | AzR | AzS | Priority | Transmitter UUID       | Mode       | Satellite name ")

    for satpass in sorted(scheduledpasses, key=lambda satpass: satpass['tr']):

#*----- Modify to add AzR and AzS

        logging.info("%3d | %3.d | %05d | %s | %s | %3.0f | %3.0f | %3.0f | %4.6f | %s | %-10s | %s" %
                (ground_station_id, satpass['scheduled'], int(satpass['id']), satpass['tr'].strftime("%Y-%m-%dT%H:%M:%S"),
                satpass['ts'].strftime("%Y-%m-%dT%H:%M:%S"), float(satpass['altt']),float(satpass['azr']),float(satpass['azs']),
                float(satpass['priority']), satpass['uuid'], satpass['mode'], satpass['name'].rstrip()))

#Aqui es la intervencion para evitar scheduling, se le pasa schedule_needed=False y a otra cosa
        if not satpass['scheduled']:
            schedule_needed = True

    # Login and schedule passes
    if schedule and schedule_needed:
        loginUrl = '{}/accounts/login/'.format(settings.NETWORK_BASE_URL)  # login URL
        session = requests.session()
        login = session.get(loginUrl)  # Get login page for CSFR token
        login_html = lxml.html.fromstring(login.text)
        login_hidden_inputs = login_html.xpath(r'//form//input[@type="hidden"]')  # Get CSFR token
        form = {x.attrib["name"]: x.attrib["value"] for x in login_hidden_inputs}
        form["login"] = settings.NETWORK_USERNAME
        form["password"] = settings.NETWORK_PASSWORD

        # Login
        result = session.post(loginUrl,
                              data=form,
                              headers={
                                  'referer': loginUrl,
                                  'user-agent': 'satnogs-auto-scheduler/0.0.1'
                              })
        if result.url.endswith("/accounts/login/"):
            logging.info("Authentication failed")
            sys.exit(-1)
        else:
            logging.info("Authentication successful")

        # Sort passes
        scheduledpasses_sorted = sorted(scheduledpasses, key=lambda satpass: satpass['tr'])

        logging.info('Checking and scheduling passes as needed.')
        for satpass in tqdm(scheduledpasses_sorted):
            if not satpass['scheduled']:
                #logging.debug("Scheduling %05d %s %s %3.0f %4.3f %s %s" %
                #logging.info("Scheduling %05d %s %s %3.0f %4.3f %s %s" %
                #              (int(satpass['id']), satpass['tr'].strftime("%Y-%m-%dT%H:%M:%S"),
                #               satpass['ts'].strftime("%Y-%m-%dT%H:%M:%S"), float(satpass['altt']),
                #               satpass['priority'], satpass['uuid'], satpass['name'].rstrip()))

#*------ Filter passes based on Az Window

                
                if (checkAz(float(satpass['azr']),azmin,azmax)) or (checkAz(float(satpass['azs']),azmin,azmax)):
                    logging.info("\n")

                    logging.info("scheduling Sat(%d) UUID(%s) Rise(%s) Set(%s)" % (int(satpass['id']),satpass['uuid'],
                                     satpass['tr'].strftime("%Y-%m-%d %H:%M:%S") + ".000",
                                     satpass['ts'].strftime("%Y-%m-%d %H:%M:%S") + ".000"))

                    schedule_observation(session, int(satpass['id']), satpass['uuid'],
                                     ground_station_id,
                                     satpass['tr'].strftime("%Y-%m-%d %H:%M:%S") + ".000",
                                     satpass['ts'].strftime("%Y-%m-%d %H:%M:%S") + ".000")
                    logging.info("\n")

                else:
                    logging.info("\n")

                    logging.info("rejecting Sat(%d) UUID(%s) Rise(%s) Set(%s)" % (int(satpass['id']),satpass['uuid'],
                                     satpass['tr'].strftime("%Y-%m-%d %H:%M:%S") + ".000",
                                     satpass['ts'].strftime("%Y-%m-%d %H:%M:%S") + ".000"))
                    logging.info("\n")


#*------------------------------------------------------------------------------------------------------------------------
        logging.info("All passes are scheduled. Exiting!")


if __name__ == '__main__':
    main()
