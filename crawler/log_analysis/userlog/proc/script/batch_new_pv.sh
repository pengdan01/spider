#!/bin/bash
set -u
source ./shell/common.sh
src=/data/userlog/url_data_v2
#dst=/app/user_data/pv_log
pvlog_dst=/app/user_data/pv_log
title_dst=/app/user_data/url_title
#$HADOOP_HOME/bin/hadoop fs -rmr ${dst}/*

parallel=1
id=1
for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
  echo $date
  [[ "$date" < "20120808" ]] && continue
  today=`date +%Y%m%d`
  [[ "$date" == "$today" ]] && break
  # bash -x gen_pv_log.sh -i $src -o $dst -q /app/user_data/md5_query/done_list -u /app/user_data/md5_url/done_list --date $date &
  bash -x proc_pv.sh --output_pv_path $pvlog_dst --output_title_path $title_dst \
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
