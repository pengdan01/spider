#!/usr/bin/python
import os
import sys

if len(sys.argv) != 5:
  print "Usage: %s fetcher_num ip_stat_file ip_to_fetcher_id_file output_file" % (sys.argv[0])
  sys.exit(1)

fetcher_url_num = [ 0 for i in range(int(sys.argv[1])) ]

ip_to_fetcher_id={}
ip_to_fetcher_id_file = open(sys.argv[3], "r")
for line in ip_to_fetcher_id_file:
  line_arr = line.split("\t")
  ip_to_fetcher_id[line_arr[0]] = int(line_arr[1])

ip_stat={}
ip_stat_file = open(sys.argv[2], "r")
for line in ip_stat_file:
  line_arr = line.split("\t")
  ip_stat[line_arr[0]] = int(line_arr[2])

for ip in ip_stat:
  fetcher_id = ip_to_fetcher_id[ip]
  url_num = ip_stat[ip]
  fetcher_url_num[fetcher_id] += url_num

output_file = open(sys.argv[4], "w")
for i in range(len(fetcher_url_num)):
  output_str = "%d\n" % fetcher_url_num[i]
  output_file.write(output_str)


