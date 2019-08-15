import requests
import logging
import math
import random
from datetime import datetime, timedelta
import ephem
import lxml
import settings
from tqdm import tqdm
import os
import sys


def get_paginated_endpoint(url, max_entries=None):
    r = requests.get(url=url)
    r.raise_for_status()

    data = r.json()

    while 'next' in r.links and (not max_entries or len(data) < max_entries):
        next_page_url = r.links['next']['url']

        r = requests.get(url=next_page_url)
        r.raise_for_status()

        data.extend(r.json())

    return data


def read_priorities_transmitters(filename):
    # Priorities and favorite transmitters
    # read the following format
    #   43017 1. KgazZMKEa74VnquqXLwAvD
    if filename is not None and os.path.exists(filename):
        with open(filename, "r") as fp:
            satprio = {}
            sattrans = {}
            lines = fp.readlines()
            for line in lines:
                if line[0]=="#":
                    continue
                parts = line.strip().split(" ")
                sat = parts[0]
                prio = parts[1]
                transmitter = parts[2]
                satprio[sat] = float(prio)
                sattrans[sat] = transmitter
        return (satprio, sattrans)
    else:
        return ({}, {})


def get_satellite_info():
    # Open session
    logging.info("Fetching satellite information from DB.")
    r = requests.get('{}/api/satellites'.format(settings.DB_BASE_URL))
    logging.info("Satellites received!")

    # Select alive satellites
    norad_cat_ids = []
    for o in r.json():
        if o["status"] == "alive":
            norad_cat_ids.append(o["norad_cat_id"])

    return norad_cat_ids


def get_active_transmitter_info(fmin, fmax):
    # Open session
    logging.info("Fetching transmitter information from DB.")
    r = requests.get('{}/api/transmitters'.format(settings.DB_BASE_URL))
    logging.info("Transmitters received!")

    # Loop
    transmitters = []
    for o in r.json():
        if o["downlink_low"]:
            if o["status"] == "active" and o["downlink_low"] > fmin and o["downlink_low"] <= fmax:
                transmitter = {"norad_cat_id": o["norad_cat_id"], "uuid": o["uuid"], "mode": o["mode"]}
                transmitters.append(transmitter)
    logging.info("Transmitters filtered based on ground station capability.")
    return transmitters


def get_transmitter_stats():
    logging.debug("Requesting transmitter success rates for all satellite")
    transmitters = get_paginated_endpoint('{}/api/transmitters/'.format(settings.NETWORK_BASE_URL))
    return transmitters


def get_scheduled_passes_from_network(ground_station, tmin, tmax):
    # Get first page
    client = requests.session()

    # Loop
    start = True
    scheduledpasses = []

    logging.info("Requesting scheduled passes for ground station %d" % ground_station)
    # Fetch observations until the time of the end of the last fetched observation happends to be
    # before the start time of the selected timerange for scheduling
    # NOTE: This algorithm is based on the order in which the API returns the observations, i.e.
    # most recent observations are returned at first!
    while True:
        if start:
            r = client.get('{}/api/observations/?ground_station={:d}'.format(
                settings.NETWORK_BASE_URL, ground_station))
            start = False
        else:
            nextpage = r.links.get("next")
            r = client.get(nextpage["url"])

        if not r.json():
            # Ground station has no observations yet
            break

        # r.json() is a list of dicts/observations
        # added empty azr and azs keys in order to properly merge later with values from computed pass and be able to filter based on az window
        for o in r.json():
            satpass = {
                "id": o['norad_cat_id'],
                "tr": datetime.strptime(o['start'].replace("Z", ""), "%Y-%m-%dT%H:%M:%S"),
                "ts": datetime.strptime(o['end'].replace("Z", ""), "%Y-%m-%dT%H:%M:%S"),
                "scheduled": True,
                "altt": o['max_altitude'],
                "priority": 1,
                "uuid": o['transmitter'],
                "name": '',
                "mode": '',
                "azr":0,        
                "azs":0
            }

            if satpass['ts'] > tmin and satpass['tr'] < tmax:
                # Only store observations which are during the ROI for scheduling
                scheduledpasses.append(satpass)

        if satpass['ts'] < tmin:
            # Last fetched observation is older than the ROI for scheduling, end loop.
            break

    logging.info("Scheduled passes for ground station %d retrieved!" % ground_station)
    return scheduledpasses


def overlap(satpass, scheduledpasses, wait_time_seconds):
    """Check if this pass overlaps with already scheduled passes"""
    # No overlap
    overlap = False

    # Add wait time
    tr = satpass['tr']
    ts = satpass['ts'] + timedelta(seconds=wait_time_seconds)

    # Loop over scheduled passes
    for scheduledpass in scheduledpasses:
        # Test pass falls within scheduled pass
        if tr >= scheduledpass['tr'] and ts < scheduledpass['ts'] + timedelta(
                seconds=wait_time_seconds):
            overlap = True
        # Scheduled pass falls within test pass
        elif scheduledpass['tr'] >= tr and scheduledpass['ts'] + timedelta(
                seconds=wait_time_seconds) < ts:
            overlap = True
        # Pass start falls within pass
        elif tr >= scheduledpass['tr'] and tr < scheduledpass['ts'] + timedelta(
                seconds=wait_time_seconds):
            overlap = True
        # Pass end falls within end
        elif ts >= scheduledpass['tr'] and ts < scheduledpass['ts'] + timedelta(
                seconds=wait_time_seconds):
            overlap = True
        if overlap:
            break

    return overlap


def ordered_scheduler(passes, scheduledpasses, wait_time_seconds):
    """Loop through a list of ordered passes and schedule each next one that fits"""
    # Loop over passes
    for satpass in passes:
        # Schedule if there is no overlap with already scheduled passes
        if not overlap(satpass, scheduledpasses, wait_time_seconds):
            scheduledpasses.append(satpass)

    return scheduledpasses


def random_scheduler(passes, scheduledpasses, wait_time_seconds):
    """Schedule passes based on random ordering"""
    # Shuffle passes
    random.shuffle(passes)

    return ordered_scheduler(passes, scheduledpasses, wait_time_seconds)


def report_efficiency(scheduledpasses, passes):
    if scheduledpasses:
        # Loop over passes
        start = False
        for satpass in scheduledpasses:
            if not start:
                dt = satpass['ts'] - satpass['tr']
                tmin = satpass['tr']
                tmax = satpass['ts']
                start = True
            else:
                dt += satpass['ts'] - satpass['tr']
                if satpass['tr'] < tmin:
                    tmin = satpass['tr']
                if satpass['ts'] > tmax:
                    tmax = satpass['ts']
        # Total time covered
        dttot = tmax - tmin

        logging.info("%d passes selected out of %d, %.0f s out of %.0f s at %.3f%% efficiency" %
                     (len(scheduledpasses), len(passes), dt.total_seconds(), dttot.total_seconds(),
                      100 * dt.total_seconds() / dttot.total_seconds()))

    else:
        logging.info("No appropriate passes found for scheduling.")


def find_passes(satellites, observer, tmin, tmax, minimum_altitude, min_pass_duration):
    # Loop over satellites
    passes = []
    passid = 0
    logging.info('Finding all passes for %s satellites:' % len(satellites))
    for satellite in tqdm(satellites):
        # Set start time
        observer.date = ephem.date(tmin)

        # Load TLE
        try:
            sat_ephem = ephem.readtle(str(satellite.tle0), str(satellite.tle1), str(satellite.tle2))
        except (ValueError, AttributeError):
            continue

        # Loop over passes
        keep_digging = True
        while keep_digging:
            sat_ephem.compute(observer)
            try:
                tr, azr, tt, altt, ts, azs = observer.next_pass(sat_ephem)
            except ValueError:
                break  # there will be sats in our list that fall below horizon, skip
            except TypeError:
                break  # if there happens to be a non-EarthSatellite object in the list
            except Exception:
                break

            if tr is None:
                break

            # using the angles module convert the sexagesimal degree into
            # something more easily read by a human
            try:
                elevation = format(math.degrees(altt), '.0f')
                azimuth_r = format(math.degrees(azr), '.0f')
                azimuth_s = format(math.degrees(azs), '.0f')
            except TypeError:
                break
            passid += 1

            pass_duration = ts.datetime() - tr.datetime()

            # show only if >= configured horizon and till tmax,
            # and not directly overhead (tr < ts see issue 199)

            if tr < ephem.date(tmax):
                if (float(elevation) >= minimum_altitude and tr < ts and
                        pass_duration > timedelta(minutes=min_pass_duration)):
                    valid = True

                    # invalidate passes that start too soon
                    if tr < ephem.Date(datetime.now() + timedelta(minutes=5)):
                        valid = False

                    # get pass information
                    satpass = {
                        'passid': passid,
                        'mytime': str(observer.date),
                        'name': str(satellite.name),
                        'id': str(satellite.id),
                        'tle1': str(satellite.tle1),
                        'tle2': str(satellite.tle2),
                        'tr': tr.datetime(),  # Rise time
                        'azr': azimuth_r,  # Rise Azimuth
                        'tt': tt.datetime(),  # Max altitude time
                        'altt': elevation,  # Max altitude
                        'ts': ts.datetime(),  # Set time
                        'azs': azimuth_s,  # Set azimuth
                        'valid': valid,
                        'uuid': satellite.transmitter,
                        'success_rate': satellite.success_rate,
                        'good_count': satellite.good_count,
                        'data_count': satellite.data_count,
                        'mode': satellite.mode,
                        'scheduled': False
                    }
                    passes.append(satpass)
                observer.date = ephem.Date(ts).datetime() + timedelta(minutes=1)
            else:
                keep_digging = False

    return passes


def get_priority_passes(passes, priorities, favorite_transmitters, only_priority, min_priority):
    priority = []
    normal = []
    for satpass in passes:
        # Is this satellite a priority satellite?
        if satpass['id'] in priorities:
            # Is this transmitter a priority transmitter?
            if satpass['uuid'] == favorite_transmitters[satpass['id']]:
                satpass['priority'] = priorities[satpass['id']]
                satpass['uuid'] = favorite_transmitters[satpass['id']]

                # Add if priority is high enough
                if satpass['priority'] >= min_priority:
                    priority.append(satpass)
        elif only_priority:
            # Find satellite transmitter with highest number of good observations
            max_good_count = max([s['good_count'] for s in passes if s["id"] == satpass["id"]])
            if max_good_count > 0:
                satpass['priority'] = \
                    (float(satpass['altt']) / 90.0) \
                    * satpass['success_rate'] \
                    * float(satpass['good_count']) / max_good_count
            else:
                satpass['priority'] = (float(satpass['altt']) / 90.0) * satpass['success_rate']

            # Add if priority is high enough
            if satpass['priority'] >= min_priority:
                normal.append(satpass)
    return (priority, normal)

def get_groundstation_info(ground_station_id, allow_testing):

    logging.info("Requesting information for ground station %d" % ground_station_id)

    # Loop
    r = requests.get("{}/api/stations/?id={:d}".format(settings.NETWORK_BASE_URL,
                                                       ground_station_id))

    selected_stations = list(filter(lambda s: s['id'] == ground_station_id, r.json()))

    if not selected_stations:
        logging.info('No ground station information found!')
        # Exit if no ground station found
        sys.exit()

    logging.info('Ground station information retrieved!')
    station = selected_stations[0]

    if station['status'] == 'Online' or (station['status'] == 'Testing' and allow_testing):
        return station
    else:
        if station['status'] == 'Testing' and not allow_testing:
            logging.info("Ground station {} is in testing mode but auto-scheduling is not "
                         "allowed. Use -T command line argument to enable scheduling.".format(ground_station_id))
        else:
            logging.info("Ground station {} neither in 'online' nor in 'testing' mode, "
                         "can't schedule!".format(ground_station_id))
        return {}


def get_last_update(fname):
    try:
        fp = open(fname, "r")
        line = fp.readline()
        fp.close()
        return datetime.strptime(line.strip(), "%Y-%m-%dT%H:%M:%S")
    except IOError:
        return None


def update_needed(tnow, ground_station_id, cache_dir):
    # Get last update
    tlast = get_last_update(os.path.join(cache_dir, "last_update_%d.txt" % ground_station_id))

    if tlast is None or (tnow - tlast).total_seconds() > settings.CACHE_AGE * 3600:
        return True
    if not os.path.isfile(os.path.join(cache_dir, "transmitters_%d.txt" % ground_station_id)):
        return True
    if not os.path.isfile(os.path.join(cache_dir, "tles_%d.txt" % ground_station_id)):
        return True
    return False


def schedule_observation(session, norad_cat_id, uuid, ground_station_id, starttime, endtime):

    obsURL = '{}/observations/new/'.format(settings.NETWORK_BASE_URL)  # Observation URL
    # Get the observation/new/ page to get the CSFR token
    obs = session.get(obsURL)
    obs_html = lxml.html.fromstring(obs.text)
    hidden_inputs = obs_html.xpath(r'//form//input[@type="hidden"]')
    form = {x.attrib["name"]: x.attrib["value"] for x in hidden_inputs}
    form["satellite"] = norad_cat_id
    form["transmitter"] = uuid
    form["start-time"] = starttime
    form["end-time"] = endtime
    form["0-starting_time"] = starttime
    form["0-ending_time"] = endtime
    form["0-station"] = ground_station_id
    form["total"] = str(1)
    session.post(obsURL, data=form, headers={'referer': obsURL})
    logging.debug("Scheduled!")
