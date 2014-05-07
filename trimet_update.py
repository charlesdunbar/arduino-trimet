#!/usr/bin/env python

# Trimet update script used to poll trimet's website for a select list of stops
# and put the format into a file that is served by nginx, which will be read
# by an arudino
# Author: Charles Dunbar
# Date: 11-3-2013
# Requires: requests (pip install requests)

import json
import requests
import sys

API_KEY = "API_KEY_HERE"
STOPS = ["STOP_NUM_HERE"]
BUSES = ["BUS_HERE", "BUS_HERE"]
parsed_data = []

# Function used to create the request URL with the api key and stops listed
# Need the stops to be comma seperated
def create_url():
        url = "http://developer.trimet.org/ws/V1/arrivals?appID=" + API_KEY \
        + "&locIDS="
        for num in STOPS:
                url += num + ","
        url += "&json=true"
        if DEBUG: print "Querying url " + url
        return url

def parse_url():
        req = requests.get(create_url())
        if req.status_code != 200: exit("Didn't receive response")
        if DEBUG: print "The returned status code is: " + str(req.status_code)
        if DEBUG: print "The returned text is: " + req.text
        data = json.loads(req.text)

        for route in data['resultSet']['arrival']: # Each item is a dict
                for k, v in route.items():
                        if DEBUG: print "key: " + str(k) + " - value: " + str(v)
                        # Match only against bus lines we care about
                        if k == "route" and str(v) in BUSES:
                                #if DEBUG: print "key: " + str(k) + " value: " + str(v)
                                if DEBUG: print "Route: " + str(route)
                                # Scheduled has a substring due to only wanting
                                # the HH:MM format of time, instead of ISO 8601
                                if "estimated" in route:
                                        parsed_data.append({"route":str(route['route']),
                                        "estimated":str(route['estimated'])[11:16],
                                        "locid":str(route['locid'])})
                                else:
                                        parsed_data.append({"route":str(route['route']),
                                        "scheduled":str(route['scheduled'])[11:16],
                                        "locid":str(route['locid'])})

def print_to_file(location):
        f = open(location,'a')
        for route in parsed_data:
                if "estimated" in route:
                        f.write(str(route['locid']) + " - " + str(route['route']) + " - " \
                        + str(route['estimated']) + "\n")
                else:
                        f.write(str(route['locid']) + " - " + str(route['route']) + " - " \
                        + str(route['scheduled']) + "\n")
        f.close()

parse_url()
if DEBUG: print "Parsed data: " + str(parsed_data)
print_to_file("path_to_web_dir_file")
