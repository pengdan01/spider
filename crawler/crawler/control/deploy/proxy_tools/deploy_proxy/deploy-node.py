#!/usr/bin/python

import os
import sys
import task_runner

def source():
  for line in open('servers.txt'):
    yield line.strip()
def run(line):
  line = line.strip()
  print line
  r = os.system('rsync -cavz shadowsocks-nodejs root@"%s":~/' % line)
  r = os.system('rsync -cavz node-debian-32bit root@"%s":~/' % line)
task_runner.map_task(source(), run, 10)



