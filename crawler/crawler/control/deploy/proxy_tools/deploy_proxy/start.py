#!/usr/bin/python

import os
import sys

for line in open('servers.txt'):
  line = line.strip()
  print line
  os.system('ssh root@"%s" "nohup python shadowsocks/server.py 1>log 2>err &"' % line)
  print 'ssh root@"%s" setsid python shadowsocks/server.py' % line


