#!/bin/bash

set -u

if [ $# -ne 2 ]; then
  echo "Usage: $0 ip port"
  exit 1
fi

source /home/crawler/pengdan/task_monitor/common.sh 

receiver="pengdan@oneboxtech.com"

target_ip=$1
target_port=$2

[ ! -d data ] && mkdir data

last_counter_file=data/${target_ip}_crawl_success_rate_counter_${target_port}
counter_file=http://${target_ip}:${target_port}/counters
local_counter_file=.${target_ip}_crawl_success_rate_counter_${target_port}

rm -f ${local_counter_file}

wget ${counter_file} -O ${local_counter_file}
if [ $? -ne 0 ]; then
  err_msg="crawler[crawl_success_ratio_${target_ip}_${target_port}]: fail in wget ${counter_file} -O ${local_counter_file}"
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi

last_timestamp=""
last_curl_error=""
last_http_2xx=""
last_http_4xx=""
last_http_5xx=""
last_task_completed=""

timestamp=`date +"%Y%m%d %H:%M:%S"`
curl_error=""
http_2xx=""
http_4xx=""
http_5xx=""
task_completed=""

## 从刚下载的文件中读取相关字段
curl_error=`cat ${local_counter_file} | grep general_crawler | grep curl_error | awk -F'\t' '{print $NF}'`
http_2xx=`cat ${local_counter_file} | grep general_crawler | grep http_2xx | awk -F'\t' '{print $NF}'`
http_4xx=`cat ${local_counter_file} | grep general_crawler | grep http_4xx | awk -F'\t' '{print $NF}'`
http_5xx=`cat ${local_counter_file} | grep general_crawler | grep http_5xx | awk -F'\t' '{print $NF}'`
task_completed=`cat ${local_counter_file} | grep general_crawler | grep task_completed | awk -F'\t' '{print $NF}'`

## 从上一个文件中读取相关字段
if [ ! -f ${last_counter_file} ]; then
  ((delta_error = curl_error))
  ((delta_2xx = http_2xx))
  ((delta_task_completed = task_completed))
else
  last_timestamp=`cat ${last_counter_file} | grep mytimestamp | awk -F'\t' '{print $NF}'`
  last_curl_error=`cat ${last_counter_file} | grep general_crawler | grep curl_error | awk -F'\t' '{print $NF}'`
  last_http_2xx=`cat ${last_counter_file} | grep general_crawler | grep http_2xx | awk -F'\t' '{print $NF}'`
  last_http_4xx=`cat ${last_counter_file} | grep general_crawler | grep http_4xx | awk -F'\t' '{print $NF}'`
  last_http_5xx=`cat ${last_counter_file} | grep general_crawler | grep http_5xx | awk -F'\t' '{print $NF}'`
  last_task_completed=`cat ${last_counter_file} | grep general_crawler | grep task_completed | awk -F'\t' '{print $NF}'`

  ((delta_error = curl_error - last_curl_error))
  ((delta_2xx = http_2xx - last_http_2xx))
  ((delta_task_completed = task_completed - last_task_completed))
fi

if [ $delta_task_completed -eq 0 ]; then
  err_msg="crawler:crawl_success_ratio_${target_ip}_${target_port}: warning, crawl 0 urls since $last_timestamp"
  SendAlertViaMail "$err_msg" "$receiver"
else
  ((curl_ok = delta_task_completed - delta_error))
  curl_ok_ratio=`echo "scale=6; $curl_ok / $delta_task_completed" | bc`
  http_2xx_ratio=`echo "scale=6; $delta_2xx / $delta_task_completed" | bc`
  # 格式化数字
  c_ok=`echo "scale=3;$curl_ok/1000" | bc`
  http_ok=`echo "scale=3;$delta_2xx/1000" | bc`
  total=`echo "scale=3;$delta_task_completed/1000" | bc`
  msg="crawler[crawl_success_ratio_${target_ip}_${target_port}]: curl_ok_ratio: ${c_ok}k/${total}k=$curl_ok_ratio, http_2xx_ratio: ${http_ok}k/${total}k=$http_2xx_ratio, since $last_timestamp"
  SendTxtReport "$msg" "$receiver" 
fi

echo -e "mytimestamp\t$timestamp" >> ${local_counter_file}
mv ${local_counter_file} ${last_counter_file}

exit 0
