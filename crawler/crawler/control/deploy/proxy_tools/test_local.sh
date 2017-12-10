#!/bin/bash

#test_url=http://www.facebook.com
test_url=http://www.youtube.com
proxy_list=proxy_ip_list

while read line
do
 [ ! "${line}" ] && continue
 local_port=`echo ${line} | awk -F':' '{print $2}'`
 ## 11111 is a special port, skip it
 [ ${local_port} -eq 11111 -o ${local_port} -eq 33333 ] && continue
 curl -I $test_url --socks5-hostname localhost:${local_port} 
 if [ $? -ne 0 ]; then
   echo "Failed in curl -I $test_url --socks5-hostname localhost:${local_port}"
   exit 1
 fi
done < ${proxy_list}
echo done!
exit 0
