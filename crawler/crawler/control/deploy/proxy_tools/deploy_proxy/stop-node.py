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
  os.system('ssh root@"%s" "killall node"' % line)

task_runner.map_task(source(), run, 10)

