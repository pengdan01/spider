#!/bin/bash
set -u
source ./shell/common.sh
src=/data/userlog/query_data_v3
searchlog_dst=/app/user_data/search_log
title_dst=/app/user_data/url_title
#$HADOOP_HOME/bin/hadoop fs -rmr ${dst}/*

parallel=1
id=1
for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
  echo $date
  [[ "$date" < "20120808" ]] && continue
  today=`date +%Y%m%d`
  [[ "$date" == "$today" ]] && break
  bash -x proc_search.sh --output_search_path $searchlog_dst --output_title_path $title_dst \
      --input_path $src --date $date &
  pid[$id]=$!
  ((id++))
  sleep 60
  if [ $id -gt $parallel ]; then
    for i in ${pid[@]}; do
      wait $i
      [[ $? -ne 0 ]] && exit 1
    done
    pid=""
    id=1
  fi
done

for i in ${pid[@]}; do
  wait $i
  [[ $? -ne 0 ]] && exit 1
done
