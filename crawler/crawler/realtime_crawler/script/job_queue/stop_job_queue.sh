#!/bin/bash
[ $# -ne 1 ] && echo "Usage: $0 queue_port_num" && exit 1
port_num=$1
line=`ps -ef | grep "bin/beanstalkd -p $port_num" | grep -v grep`
if [ "$line" ]; then
  pid1=`echo $line | awk '{print $2}'`
  pid2=`echo $line | awk '{print $3}'`
  kill -9 $pid1 $pid2
fi
exit 0
