#! /usr/bin/python
import sys
import os

if len(sys.argv) != 4:
  print "Usage: %s sorted_ip_count fetcher_num output_file" % (sys.argv[0])
  sys.exit(1)

fetcher_num = int(sys.argv[2])
total_ip_count = 0.0
average_ip_count = 0.0
# read ip->count file, calc total count
ip_file = open(sys.argv[1], "r")
for line in ip_file:
  line_arr = line.split("\t")
  this_ip_count = float(line_arr[2])
  total_ip_count += this_ip_count
average_ip_count = total_ip_count / fetcher_num
ip_file.close()

current_node_idx = 0
current_ip_count = 0.0
ip_file = open(sys.argv[1], "r")
output_file = open(sys.argv[3], "w")
for line in ip_file:
  line_arr = line.split("\t")
  output_value = "%s\t%d" % (line_arr[0], current_node_idx)
  output_file.write(output_value+"\n")
  this_ip_ip_count = float(line_arr[2])
  current_ip_count += this_ip_ip_count
  if current_ip_count >= average_ip_count * (current_node_idx + 1):
    current_node_idx += 1
output_file.close()
ip_file.close()

