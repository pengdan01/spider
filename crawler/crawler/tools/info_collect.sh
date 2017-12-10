#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 hosts_info/crawler_info.txt"
  exit 1
fi
counter_dir=wly_crawler_online/.build/opt/targets/samples/collect_counters

hosts_file=$1

if [ ! -e ${hosts_file} ]; then
  echo "crawler_hosts_file[${hosts_file}] not exist"
  exit 1
fi

port=19832

grep -v "^#" ${hosts_file} | sed /'^ *$'/d | awk -v a=${port} '{printf("%s:%s\n", $1,a)}' > hostfile.tmp

hosts_string=""

while read line
do
  if [ $hosts_string ]; then
    hosts_string="$hosts_string,$line"
  else
    hosts_string="$line"
  fi
done < hostfile.tmp &>/dev/null

rm -f /tmp/* 2>/dev/null

${counter_dir}/collect_counters --hosts="${hosts_string}" --log_dir=/tmp

rm -f hostfile.tmp 
exit 0
