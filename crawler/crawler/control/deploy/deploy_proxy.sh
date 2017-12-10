#!/bin/bash

[ $# -ne 1 ] && echo "Usage: $0 host_list_file" && exit 1

host_list_file=$1

[ ! -f ${host_list_file} ] && echo "host list file[${host_list_file}] not exist" && exit 1

## Delete the old proxy_tools dir
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "rm -rf /home/crawler/proxy_tools"
if [ $? -ne 0 ]; then
  echo "Failed: pssh -h ${host_list_file} -t 0 'rm -rf /home/crawler/proxy_tools'"
  exit 1
fi

## cp new proxy_tools to each node 
build_tools/pssh/pscp -h ${host_list_file} -r proxy_tools /home/crawler/
if [ $? -ne 0 ]; then
  echo "Failed: pscp -h ${host_list_file} -r proxy_tools /home/crawler/"
  exit 1
fi

## kill all local.py runing on each node 
build_tools/pssh/pnuke -h ${host_list_file} -t 0 -p 128 python
build_tools/pssh/pnuke -h ${host_list_file} -t 0 -p 128 node 

build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "rm -f /dev/shm/local_proxy*"

## start local.py on each node 
cmd="cd /home/crawler/proxy_tools && bash -x start_local.sh"
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "$cmd"
if [ $? -ne 0 ]; then
  echo "Failed: pssh -h ${host_list_file} -t 0 $cmd"
  exit 1
fi

echo "done!"
exit 0
