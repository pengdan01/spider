#!/bin/bash
set -u
source ./shell/common.sh

src=/data/userlog/query_data_v3
log_dst=/app/user_data/search_log
title_dst=/app/user_data/url_title
alert_mail=zhengying@oneboxtech.com

while [ 1 ]; do 
  today=`date +%Y%m%d`
  for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
    echo $date
    [[ "$date" == "$today" ]] && break

    final_output="${log_dst}/${date}_00_00.st" 
    ${HADOOP_HOME}/bin/hadoop fs -test -d ${final_output}
    [[ $? -eq 0 ]] && continue

    bash -x proc_search.sh --output_search_path $log_dst --output_title_path $title_dst \
        --input_path $src --date $date
    [[ $? -ne 0 ]] && SendAlertViaMailAndDie "proc search log failed" "${alert_mail}"
  done
  sleep 86400
done

