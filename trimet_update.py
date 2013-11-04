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
	return url

def parse_url():
	req = requests.get(create_url())
	if req.status_code != 200: exit("Didn't receive response")
	data = json.loads(req.text)

	for route in data['resultSet']['arrival']: # Each item is a dict
		for k, v in route.items():
			# Match only against bus lines we care about
			if k == "route" and str(v) in BUSES:
				# Scheduled has a substring due to only wanting
				# the HH:MM format of time, instead of ISO 8601
				parsed_data.append({"route":str(route['route']),
				"scheduled":str(route['scheduled'])[11:16],
				"locid":str(route['locid'])})

def print_to_file(location):
    # Prints to file in the format of
	# STOP_NUM - BUS_LINE - SCHEDULED_ARRIVAL_TIME
	f = open(location,'a')
	for route in parsed_data:
		f.write(str(route['locid']) + " - " + str(route['route']) + " - " \
		+ str(route['scheduled']) + "\n")
	f.close()

parse_url()
print_to_file("path_to_web_dir_file")
