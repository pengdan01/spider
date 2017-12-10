#!/bin/bash
set -u
source ./shell/common.sh

src=/app/user_data/pv_log
dst=/app/user_data/url_query
alert_mail=zhengying@oneboxtech.com

newest_date=`$HADOOP_HOME/bin/hadoop fs -ls $dst | awk 'NF > 5' | awk -F'.' '$NF=="st"' | awk -F'/' '{print $NF;}' | tail -1 | awk -F'_' '{print $1;}'`
echo $newest_date
while [ 1 ]; do 
  today=`date +%Y%m%d`
  for date in `$HADOOP_HOME/bin/hadoop fs -ls $src | awk 'NF > 5' | awk -F'/' '{print $NF;}'`;do
    date=`echo $date | awk -F'_' '{print $1;}'`
    echo $date
    [[ "$date" < "$newest_date" ]] && continue
    [[ "$date" == "$today" ]] && break

    final_output="${dst}/${date}_00_00.st" 
    ${HADOOP_HOME}/bin/hadoop fs -test -d ${final_output}
    [[ $? -eq 0 ]] && continue

    bash -x gen_url_query.sh -o $dst -i $src --date $date 
    [[ $? -ne 0 ]] && SendAlertViaMailAndDie "proc pv log failed" "${alert_mail}"
    
  done

  sleep 86400
done

