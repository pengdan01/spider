#!/usr/bin/python

import os
import sys

for line in open('servers.txt'):
  line = line.strip()
  print line
  os.system('ssh root@"%s" killall python' % line)


