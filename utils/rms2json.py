#!/usr/bin/env python3
import json
import csv
import datetime
import sys
import os

#INPUT_FILE = "YULB.TW..HHE.2020.035.rms"
INPUT_DATE_FMT = "%Y,%j,%H:%M:%S"
OUTPUT_DATE_FMT = "%Y-%m-%dT%H:%M:%S"

output_dict = {}
data_list = []
start_time_stamp = datetime.datetime(1970, 1, 1)
network = ''
station = ''
location = ''
channel = ''

try:
    input_rms_file =  sys.argv[1]
except IndexError:
    raise SystemExit(f"Usage: {sys.argv[0]} <input rms file>")
output_json_file = os.path.splitext(input_rms_file)[0] + '.json'

with open(input_rms_file) as f:
    spamreader = csv.reader(f)
    line_count = 1

    for row in spamreader:
        # Line #1 contains the start timestamp
        # and the attributes of this record
        if line_count == 1:
            start_time_stamp = datetime.datetime.strptime(row[0], INPUT_DATE_FMT)
            station = row[1]
            network = row[2]
            channel = row[3]
            location = row[4]

            output_dict["network"] = network
            output_dict["station"] = station
            output_dict["location"] = location
            output_dict["channel"] = channel

            line_count += 1
        else:
            time_stamp = start_time_stamp + datetime.timedelta(seconds=int(row[0]))
            mean = row[1]
            rms = row[2]
            minimum = row[3]
            maximum = row[4]
            min_demean = row[5]
            max_demean = row[6]

            each_data_dict = {
                "timestamp": time_stamp.strftime(OUTPUT_DATE_FMT),
                "mean": float(mean),
                "rms": float(rms),
                "min": float(minimum),
                "max": float(maximum),
                "minDemean": float(min_demean),
                "maxDemean": float(max_demean)
            }
            data_list.append(each_data_dict)

            line_count += 1

output_dict["data"] = data_list
with open(output_json_file, 'w') as outfile:
    json.dump(output_dict, outfile)
