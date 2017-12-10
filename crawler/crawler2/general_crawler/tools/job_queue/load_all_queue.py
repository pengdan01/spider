#!/usr/bin/python
import os


for port in [10080,10081, 10020]:
  os.system(" beanstalkd.%s/bin/load.sh stop " %(port))
  os.system(" beanstalkd.%s/bin/load.sh load" %(port))
  print 'start port',port




