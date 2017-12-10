#!/usr/bin/python
import os
import sys

if len(sys.argv) != 3:
  print "Usage: %s dns_output_dir output_file"  % (sys.argv[0])
  sys.exit(1)
ips = {}

output_file = open(sys.argv[2], "w")

for item in os.listdir(sys.argv[1]):
  ips.clear()
  subpath = os.path.join(sys.argv[1], item)
  f = open(subpath, "r")
  for line in f:
    line_arr = line[:-1].split("\t")
    ips[line_arr[0]] = 1
  output_string = item;
  for ip in ips:
    output_string += "\t%s" % (ip)
  output_string += "\n"
  output_file.write(output_string)
