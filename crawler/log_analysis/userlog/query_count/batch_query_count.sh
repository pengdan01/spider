#!/bin/bash
set -u

today=`date +%Y%m%d`
# src=/data/userlog/upload_query
# dst=/app/user_data/md5_query/
# 
# for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
#   echo $date
#   [[ "$date" < "20120322" ]] && continue
#   if [ "$date" != "$today" ]; then
#     bash -x gen_md5_map.sh -o $dst -i $src --date $date --query_or_url 0 --reducer 50
#     [[ $? -ne 0 ]] && exit 1
#   fi
# done

src=/app/user_data/pv_log
dst=/app/user_data/url_query
#$HADOOP_HOME/bin/hadoop fs -rmr ${dst}/*

for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
  date=`echo $date | awk -F'_' '{print $1;}'`
  echo $date
  [[ $date < "20120617" ]] && continue
  # [[ $date == "20120401" ]] && break
  if [ "$date" != "$today" ]; then
    bash -x gen_url_query.sh -o $dst -i $src --date $date 
    [[ $? -ne 0 ]] && exit 1
  fi
done
