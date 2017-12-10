#!/usr/bin/python

import os
import sys

for line in open('servers.txt'):
  line = line.strip()
  print line
  r = os.system('rsync -cavz shadowsocks root@"%s":~/' % line)
  if r != 0:
    r = os.system('ssh root@"%s" apt-get install rsync' % line)
    os.system('rsync -cavz shadowsocks root@"%s":~/' % line)



