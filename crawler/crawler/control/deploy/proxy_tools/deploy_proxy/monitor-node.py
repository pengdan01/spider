#!/usr/bin/python

import os
import sys
import time

ports = []
port_to_proxy = {}

port = 8500

for line in open('servers.txt'):
  line = line.strip()
  print line
  os.system('node-centos-64bit/node shadowsocks-nodejs/local.js %s %d &' % (line, port))
  ports.append(port)
  port_to_proxy[port] = line
  port += 1

print 'waiting for proxy for 10 seconds...'
time.sleep(10)

for port in ports:
  r = os.system('curl http://www.youtube.com/ --socks5-hostname 127.0.0.1:%d > /dev/null' % port)
  if r != 0:
    proxy = port_to_proxy[port]
    print >> sys.stderr, 'proxy %s failed test, relaunching...' % proxy
    os.system('ssh root@%s killall node' % proxy)
    os.system('ssh root@"%s" "nohup ~/node-debian-32bit/node shadowsocks-nodejs/server.js 1>node_log 2>node_err &"' % proxy)

print 'test passed'
os.system("kill `ps aux|grep local.js|awk '{print $2}'`")
sys.exit(0)
  
