import os
import sys

new_stat = {}
old_stat = {}

if len(sys.argv) != 3:
  print "Usage: %s new_stat_file old_stat_file" % sys.argv[0]

new_stat_file = open(sys.argv[1], "r")
for line in new_stat_file:
  line_arr = line[:-1].split("\t")
  new_stat[line_arr[0]] = line

old_stat_file = open(sys.argv[2], "r")
for line in old_stat_file:
  line_arr = line[:-1].split("\t")
  old_stat[line_arr[0]] = line

print "Date\tip\turl_num\thost_num\thost1\thost1_url_num\thost2\thost2_url_num"
for ip in new_stat:
  print "Today:   \t%s" % new_stat[ip],
  if ip in old_stat:
    print "Yestoday:\t%s" % old_stat[ip],
    del old_stat[ip]
  else:
    print "Yestoday:\tNULL",

for ip in old_stat:
  print "Today:   \tNULL",
  print "Yestoday:\t%s" % old_stat[ip],
