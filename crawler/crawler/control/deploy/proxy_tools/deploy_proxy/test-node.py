#!/usr/bin/python

import os
import sys
import time

ports = []

port = 8500

for line in open('servers.txt'):
  line = line.strip()
  print line
  os.system('node-centos-64bit/node shadowsocks-nodejs/local.js %s %d &' % (line, port))
  ports.append(port)
  port += 1

print 'waiting for proxy for 7 seconds...'
time.sleep(7)

for port in ports:
  r = os.system('curl http://www.youtube.com/ --socks5-hostname 127.0.0.1:%d > /dev/null' % port)
  if r != 0:
    print >> sys.stderr, 'test failed'
    os.system("kill `ps aux|grep local.js|awk '{print $2}'`")
    sys.exit(1)

print 'test passed'
os.system("kill `ps aux|grep local.js|awk '{print $2}'`")
sys.exit(0)
  
