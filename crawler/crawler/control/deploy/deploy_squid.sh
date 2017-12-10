#!/bin/bash

[ $# -ne 1 ] && echo "Usage: $0 host_list_file" && exit 1

host_list_file=$1

[ ! -f ${host_list_file} ] && echo "host list file[${host_list_file}] not exist" && exit 1

## kill all squid runing on each node 
build_tools/pssh/pnuke -h ${host_list_file} -t 0 -p 128 squid 

## Delete the old proxy_tools dir
#build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "rm -rf /home/crawler/squid"
#if [ $? -ne 0 ]; then
#  echo "Failed: pssh -h ${host_list_file} -t 0 -p 128 'rm -rf /home/crawler/squid'"
#  exit 1
#fi
#build_tools/pssh/pssh -h ${host_list_file} -t 0  -p 128 "rm -rf /home/crawler/tmp/squid"
#build_tools/pssh/pssh -h ${host_list_file} -t 0  -p 128 "mkdir -p  /home/crawler/tmp/"
#
### cp new proxy_tools to each node 
#build_tools/pssh/pscp -h ${host_list_file} -r squid /home/crawler/tmp/
#if [ $? -ne 0 ]; then
#  echo "Failed: pscp -h ${host_list_file} -r squid /home/crawler/tmp/"
#  exit 1
#fi
#

## start local.py on each node 
#cmd="cd /home/crawler/tmp/squid && bash -x install.sh &>squid.log"
cmd="/home/crawler/squid/squid/sbin/squid"
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 -i "$cmd"
if [ $? -ne 0 ]; then
  echo "Failed: pssh -h ${host_list_file} -t 0 $cmd"
  exit 1
fi

echo "done!"
exit 0
