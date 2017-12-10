#!/bin/bash
set -u

## The script is used to deploy wly_crawler_online to the crawler cluster
## and this script should be executed at same directory where wly_crawler_online stay 
pssh_home_dir=wly_crawler_online/build_tools/pssh
target_dir="/home/pengdan/wly_crawler_online"

#${pssh_home_dir}/pssh -A -i -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no \
${pssh_home_dir}/pssh -i -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no -t 0 \
"rm -rf ${target_dir}/core* ${target_dir}/.build ${target_dir}/crawler ${target_dir}/build_tools \
${target_dir}/hosts_info"
if [ $? -ne 0 ]; then
  echo "pssh rm fail"
  exit 1
fi
#${pssh_home_dir}/pscp -r -A -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no \
${pssh_home_dir}/pscp -r -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no -t 0 \
wly_crawler_online/.build ${target_dir}/
if [ $? -ne 0 ]; then
  echo "pscp .build fail"
  exit 1
fi
${pssh_home_dir}/pscp -r -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no -t 0 \
wly_crawler_online/build_tools ${target_dir}/
if [ $? -ne 0 ]; then
  echo "pscp build_tools fail"
  exit 1
fi
${pssh_home_dir}/pscp -r -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no -t 0 \
wly_crawler_online/crawler ${target_dir}/
if [ $? -ne 0 ]; then
  echo "pscp crawler fail"
  exit 1
fi
#${pssh_home_dir}/pscp -r -A -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no \
${pssh_home_dir}/pscp -r -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no -t 0 \
hosts_info ${target_dir}/hosts_info 
if [ $? -ne 0 ]; then
  echo "pssh hosts_info fail"
  exit 1
fi

echo "Delply Done!"
exit 0
