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
  os.system('ssh root@"%s" "nohup ~/node-debian-32bit/node shadowsocks-nodejs/server.js 1>node_log 2>node_err &"' % line)

task_runner.map_task(source(), run, 20)

