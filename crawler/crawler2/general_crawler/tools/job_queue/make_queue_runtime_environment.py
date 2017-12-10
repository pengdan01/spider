#!/usr/bin/python
import os


for port in [10080,10081, 10020]:
  os.system(' mkdir -p beanstalkd.%s/bin ' %(port))
  os.system(' mkdir -p beanstalkd.%s/data ' %(port))
  os.system(' cp .bin/beanstalkd beanstalkd.%s/bin/beanstalkd.%s' %(port,port))
  
  runfd = file('beanstalkd.%s/bin/run'%(port), 'w')
  runfd.write("#!/bin/bash\n")
  runfd.write("exec ./beanstalkd.%s -p %s -z 10240000 -s 30485760 -b ../data\n" %(port,port))
  runfd.close()
  os.system(' chmod +x beanstalkd.%s/bin/run'%(port))
  os.system("cp load.sh beanstalkd.%s/bin" %(port))




