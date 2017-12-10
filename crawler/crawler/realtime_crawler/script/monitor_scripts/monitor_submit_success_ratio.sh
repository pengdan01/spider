#!/bin/bash

set -u

source ~/workspace/shell/common.sh

#receiver="pengdan@oneboxtech.com yanglei@oneboxtech.com chenxiaofeng@oneboxtech.com suhua@oneboxtech.com lintao@oneboxtech.com"
receiver="pengdan@oneboxtech.com"

#target_ip="180.153.227.166 10.115.80.167 10.115.86.99"
target_ip="180.153.227.166 10.115.80.167 10.115.86.99"
target_port=8090

declare -a submit_total_array
declare -a submit_success_array

cnt=0
for ip in `echo $target_ip`
do
last_counter_file=data/${ip}_realtime_extracter_counter

counter_file=http://${ip}:${target_port}/counters

local_counter_file=.${ip}_submit_success_rate_counter_file

rm -f ${local_counter_file}

wget ${counter_file} -O ${local_counter_file}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:submit_success_ratio_monitor: fail in wget ${counter_file} -O ${local_counter_file}"
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi

last_timestamp=""
last_submit_total=0
last_submit_success=0

timestamp=`date +"%Y%m%d %H:%M:%S"`
submit_total=0
submit_success=0

## 从刚下载的文件中读取相关字段
submit_total=`cat ${local_counter_file} | grep index_submitter | grep submitted_packets | awk -F'\t' '{print $NF}'`
submit_success=`cat ${local_counter_file} | grep index_submitter | grep submitted_success_packets | awk -F'\t' '{print $NF}'`

## 从上一个文件中读取相关字段
if [ ! -f ${last_counter_file} ]; then
  submit_total_array[$cnt]=$submit_total 
  submit_success_array[$cnt]=$submit_success
else
  last_timestamp=`cat ${last_counter_file} | grep mytimestamp | awk -F'\t' '{print $NF}'`
  last_submit_total=`cat ${last_counter_file} | grep index_submitter | grep submitted_packets | awk -F'\t' '{print $NF}'`
  last_submit_success=`cat ${last_counter_file} | grep index_submitter | grep submitted_success_packets | awk -F'\t' '{print $NF}'`

  ((delta_total = submit_total - last_submit_total))
  ((delta_success = submit_success - last_submit_success))
  submit_total_array[$cnt]=$delta_total 
  submit_success_array[$cnt]=$delta_success
fi

echo -e "mytimestamp\t$timestamp" >> ${local_counter_file}
mv ${local_counter_file} ${last_counter_file}

((cnt++))
done

## 分析数组, 生成邮件
((cnt--))
s_total=0
s_succ=0
sub_msg="each ip: "
for i in `seq 0 $cnt` 
do
  delta_t=${submit_total_array[$i]}
  delta_s=${submit_success_array[$i]}
  ((s_total = s_total + delta_t))
  ((s_succ = s_succ + delta_s))
  if [ ${submit_total_array[$i]} -eq 0 -o ${submit_success_array[$i]} -eq 0 ]; then
    sub_msg="$sub_msg, ${submit_success_array[$i]}/${submit_total_array[$i]}"
  else
    sub_success_ratio=`echo "scale=6; ${submit_success_array[$i]}/${submit_total_array[$i]}" | bc`
    sub_msg="$sub_msg, ${submit_success_array[$i]}/${submit_total_array[$i]}=$sub_success_ratio"
  fi
done
if [ $s_total -eq 0 ]; then
  msg="realtime_crawler:submit_success_ratio_monitor: submit 0 tasks"
  SendTxtReport "$msg" "$receiver" 
  exit 0
fi
success_ratio=`echo "scale=6; $s_succ/$s_total" | bc`
# 格式化数字
s=`echo "scale=3; $s_succ/1000" | bc`
t=`echo "scale=3; $s_total/1000" | bc`
msg="realtime_crawler[submit_success_ratio_monitor] submit_success_ratio: ${s}k/${t}k=$success_ratio, since $last_timestamp, $sub_msg"
SendTxtReport "$msg" "$receiver" 

exit 0
