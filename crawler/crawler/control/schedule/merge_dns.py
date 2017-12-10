#! /usr/bin/python

import sys
import os
import string

if len(sys.argv) != 5:
  print "Usage: %s dns_file1 dns_file2 black_list output_file"
  sys.exit(1)

black_list = {}
black_list_fp = open(sys.argv[3], "r")
for line in black_list_fp:
  line_arr = line[:-1].split("\t", 1)
  black_list[line_arr[0]] = line_arr[1]

host_dns = {}

file1 = open(sys.argv[1], "r")
for line in file1:
  line_arr = line[:-1].split("\t")
  host_dns[line_arr[0]] = line[:-1]

file2 = open(sys.argv[2], "r")
for line in file2:
  line_arr = line[:-1].split("\t")
  if line_arr[0] in host_dns:
    file1_arr = host_dns[line_arr[0]].split("\t")
    for i in range(1, len(line_arr)):
      if line_arr[i] not in file1_arr:
        host_dns[line_arr[0]] += "\t"
        host_dns[line_arr[0]] += line_arr[i]
  else:
    host_dns[line_arr[0]] = line[:-1]

for key,value in black_list.items():
  black_ip_list = value.split("\t")
  ip_list = host_dns[key].split("\t")
  new_ip_list = []
  for ip in ip_list:
    if ip not in black_ip_list:
      new_ip_list.append(ip)
  new_string = string.join(new_ip_list, "\t")
  host_dns[key] = new_string;

output_file = open(sys.argv[4], "w")
for dns in host_dns:
  output_file.write(host_dns[dns]+"\n")

