#!/bin/bash

start_cmd="deploy_proxy/node-centos-64bit/node deploy_proxy/shadowsocks-nodejs/local.js"
proxy_list=proxy_ip_list
proxy_list_tmp=${proxy_list}_tmp

cat ${proxy_list} | grep -v "^#" > ${proxy_list_tmp}

while read line
do
 [ ! "${line}" ] && continue
 server_ip=`echo ${line} | awk -F':' '{print $1}'`
 server_port=`echo ${line} | awk -F':' '{print $2}'`
 ${start_cmd} ${server_ip} ${server_port} &>/dev/shm/local_proxy.${server_port}.log&
 if [ $? -ne 0 ]; then
  echo "Failed in ${start_cmd} ${server_ip} ${server_port}"
  exit 1
 fi
 echo "done for ${server_ip} ${server_port}"
done < ${proxy_list_tmp} 

rm -f ${proxy_list_tmp}

echo done!
exit 0
