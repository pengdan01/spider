#! /usr/bin/python
import sys
import os

if len(sys.argv) != 4:
  print "Usage: %s sorted_host_time_consume_file fetcher_num output_file" % (sys.argv[0])
  sys.exit(1)

fetcher_num = int(sys.argv[2])
total_time_consume = 0.0
average_time_consume = 0.0
# read host->time_consume file, calc total time to be consumed
host_file = open(sys.argv[1], "r")
for line in host_file:
  line_arr = line.split("\t")
  this_host_time_consume = float(line_arr[1])
  total_time_consume += this_host_time_consume
average_time_consume = total_time_consume / fetcher_num
host_file.close()

current_node_idx = 0
current_time_consume = 0.0
host_file = open(sys.argv[1], "r")
output_file = open(sys.argv[3], "w")
for line in host_file:
  line_arr = line.split("\t")
  output_value = "%s\t%d" % (line_arr[0], current_node_idx)
  output_file.write(output_value+"\n")
  this_host_time_consume = float(line_arr[1])
  current_time_consume += this_host_time_consume
  if current_time_consume >= average_time_consume * (current_node_idx + 1):
    current_node_idx += 1
output_file.close()
host_file.close()
