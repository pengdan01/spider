#!/bin/bash
set -u
source ~/workspace/shell/common.sh

receiver="pengdan@oneboxtech.com"

## m12 上部署了unique url(15000) 数 和 hub_page/2/3(16008 and 16009) 数
target_ip1="10.115.80.167"
target_ip1_port1=15000
target_ip1_port2=16008
target_ip1_port3=16009
target_ip1_port4=16000

## t98 上部署了 时效性相关
target_ip2="10.115.86.98"
## search engine page monitor
target_ip2_port1=17001
## url time series search log 
target_ip2_port2=18001
## url time series
target_ip2_port3=19000

# global_url_uniq
last_global_url_uniq_counter_file=data/${target_ip1}_${target_ip1_port1}_global_url_uniq
global_url_uniq_counter_file=http://${target_ip1}:${target_ip1_port1}/counters
local_global_url_uniq_counter_file=.${target_ip1}_${target_ip1_port1}_global_url_uniq
rm -f ${local_global_url_uniq_counter_file}

wget ${global_url_uniq_counter_file} -O ${local_global_url_uniq_counter_file}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${global_url_uniq_counter_file} -O ${local_global_url_uniq_counter_file}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
dup_urls=`cat ${local_global_url_uniq_counter_file} | grep global_url_uniq | grep dup_urls | awk -F'\t' '{print $NF}'`
uniq_urls=`cat ${local_global_url_uniq_counter_file} | grep global_url_uniq | grep uniq_urls | awk -F'\t' '{print $NF}'`

delta_dup_urls=0
delta_uniq_urls=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_global_url_uniq_counter_file} ]; then
  delta_dup_urls=$dup_urls 
  delta_uniq_urls=$uniq_urls
else
  last_dup_urls=`cat ${last_global_url_uniq_counter_file} | grep global_url_uniq | grep dup_urls | awk -F'\t' '{print $NF}'`
  last_uniq_urls=`cat ${last_global_url_uniq_counter_file} | grep global_url_uniq | grep uniq_urls | awk -F'\t' '{print $NF}'`
  ((delta_dup_urls = dup_urls - last_dup_urls))
  ((delta_uniq_urls = uniq_urls - last_uniq_urls))
fi

# hub_page_monitor(port2: 16008)
last_hub_page_monitor_counter_file=data/${target_ip1}_${target_ip1_port2}_hub_page_monitor
hub_page_monitor_counter_file=http://${target_ip1}:${target_ip1_port2}/counters
local_hub_page_monitor_counter_file=.${target_ip1}_${target_ip1_port2}_hub_page_monitor
rm -f ${local_hub_page_monitor_counter_file}

wget ${hub_page_monitor_counter_file} -O ${local_hub_page_monitor_counter_file}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${hub_page_monitor_counter_file} -O ${local_hub_page_monitor_counter_file}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
hub_page_urls=`cat ${local_hub_page_monitor_counter_file} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`

delta_hub_page_urls=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_hub_page_monitor_counter_file} ]; then
  delta_hub_page_urls=$hub_page_urls
else
  last_hub_page_urls=`cat ${last_hub_page_monitor_counter_file} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`
  ((delta_hub_page_urls = hub_page_urls - last_hub_page_urls))
fi

# hub_page_monitor(port3: 16009)
last_hub_page_monitor_counter_file2=data/${target_ip1}_${target_ip1_port3}_hub_page_monitor
hub_page_monitor_counter_file2=http://${target_ip1}:${target_ip1_port3}/counters
local_hub_page_monitor_counter_file2=.${target_ip1}_${target_ip1_port3}_hub_page_monitor
rm -f ${local_hub_page_monitor_counter_file2}

wget ${hub_page_monitor_counter_file2} -O ${local_hub_page_monitor_counter_file2}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${hub_page_monitor_counter_file2} -O ${local_hub_page_monitor_counter_file2}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
hub_page_urls2=`cat ${local_hub_page_monitor_counter_file2} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`

delta_hub_page_urls2=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_hub_page_monitor_counter_file2} ]; then
  delta_hub_page_urls2=$hub_page_urls2
else
  last_hub_page_urls2=`cat ${last_hub_page_monitor_counter_file2} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`
  ((delta_hub_page_urls2 = hub_page_urls2 - last_hub_page_urls2))
fi

# hub_page_monitor(port4: 16000)
last_hub_page_monitor_counter_file3=data/${target_ip1}_${target_ip1_port4}_hub_page_monitor
hub_page_monitor_counter_file3=http://${target_ip1}:${target_ip1_port4}/counters
local_hub_page_monitor_counter_file3=.${target_ip1}_${target_ip1_port4}_hub_page_monitor
rm -f ${local_hub_page_monitor_counter_file3}

wget ${hub_page_monitor_counter_file3} -O ${local_hub_page_monitor_counter_file3}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${hub_page_monitor_counter_file3} -O ${local_hub_page_monitor_counter_file3}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
hub_page_urls3=`cat ${local_hub_page_monitor_counter_file3} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`

delta_hub_page_urls3=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_hub_page_monitor_counter_file3} ]; then
  delta_hub_page_urls3=$hub_page_urls3
else
  last_hub_page_urls3=`cat ${last_hub_page_monitor_counter_file3} | grep hub_page_monitor | grep urls | awk -F'\t' '{print $NF}'`
  ((delta_hub_page_urls3 = hub_page_urls3 - last_hub_page_urls3))
fi

# search engine page monitor 
last_search_engine_page_counter_file=data/${target_ip2}_${target_ip2_port1}_search_engine_page
search_engine_page_counter_file=http://${target_ip2}:${target_ip2_port1}/counters
local_search_engine_page_counter_file=.${target_ip2}_${target_ip2_port1}_search_engine_page
rm -f ${local_search_engine_page_counter_file}

wget ${search_engine_page_counter_file} -O ${local_search_engine_page_counter_file}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${search_engine_page_counter_file} -O ${local_search_engine_page_counter_file}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
search_engine_page_urls=`cat ${local_search_engine_page_counter_file} | grep search_engine_page_monitor | grep urls | awk -F'\t' '{print $NF}'`

delta_search_engine_page_urls=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_search_engine_page_counter_file} ]; then
  delta_search_engine_page_urls=$search_engine_page_urls
else
  last_search_engine_page_urls=`cat ${last_search_engine_page_counter_file} | grep search_engine_page_monitor | grep urls | awk -F'\t' '{print $NF}'` 
  ((delta_search_engine_page_urls = search_engine_page_urls - last_search_engine_page_urls))
fi

# url_time_series_search_log 
last_url_time_series_search_log_counter_file=data/${target_ip2}_${target_ip2_port2}_url_time_series_search_log
url_time_series_search_log_counter_file=http://${target_ip2}:${target_ip2_port2}/counters
local_url_time_series_search_log_counter_file=.${target_ip2}_${target_ip2_port2}_url_time_series_search_log
rm -f ${local_url_time_series_search_log_counter_file}

wget ${url_time_series_search_log_counter_file} -O ${local_url_time_series_search_log_counter_file}
if [ $? -ne 0 ]; then
  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${url_time_series_search_log_counter_file} -O ${local_url_time_series_search_log_counter_file}" 
  SendAlertViaMail "$err_msg" "$receiver" 
  exit 1
fi
## 从刚下载的文件中读取相关字段
url_time_series_search_log_urls=`cat ${local_url_time_series_search_log_counter_file} | grep url_time_series_search_log | grep urls | awk -F'\t' '{print $NF}'`

delta_url_time_series_search_log_urls=0
## 从上一个文件中读取相关字段
if [ ! -f ${last_url_time_series_search_log_counter_file} ]; then
  delta_url_time_series_search_log_urls=$url_time_series_search_log_urls
else
  last_url_time_series_search_log_urls=`cat ${last_url_time_series_search_log_counter_file} | grep url_time_series_search_log | grep urls | awk -F'\t' '{print $NF}'` 
  ((delta_url_time_series_search_log_urls = url_time_series_search_log_urls - last_url_time_series_search_log_urls))
fi

# url_time_series 
#last_url_time_series_counter_file=data/${target_ip2}_${target_ip2_port3}_url_time_series
#url_time_series_counter_file=http://${target_ip2}:${target_ip2_port3}/counters
#local_url_time_series_counter_file=.${target_ip2}_${target_ip2_port3}_url_time_series
#rm -f ${local_url_time_series_counter_file}
#
#wget ${url_time_series_counter_file} -O ${local_url_time_series_counter_file}
#if [ $? -ne 0 ]; then
#  err_msg="realtime_crawler:issue_selected_urls_stat: fail in wget ${url_time_series_counter_file} -O ${local_url_time_series_counter_file}" 
#  SendAlertViaMail "$err_msg" "$receiver" 
#  exit 1
#fi
### 从刚下载的文件中读取相关字段
#url_time_series_urls=`cat ${local_url_time_series_counter_file} | grep url_time_series | grep urls | awk -F'\t' '{print $NF}'`
#
delta_url_time_series_urls=0
### 从上一个文件中读取相关字段
#if [ ! -f ${last_url_time_series_counter_file} ]; then
#  delta_url_time_series_urls=$delta_url_time_series_urls
#else
#  last_url_time_series_urls=`cat ${last_url_time_series_counter_file} | grep url_time_series | grep urls | awk -F'\t' '{print $NF}'` 
#  ((delta_url_time_series_urls = url_time_series_urls - last_url_time_series_urls))
#fi

mv ${local_global_url_uniq_counter_file} ${last_global_url_uniq_counter_file}
mv ${local_hub_page_monitor_counter_file} ${last_hub_page_monitor_counter_file}
mv ${local_hub_page_monitor_counter_file2} ${last_hub_page_monitor_counter_file2}
mv ${local_hub_page_monitor_counter_file3} ${last_hub_page_monitor_counter_file3}
mv ${local_search_engine_page_counter_file} ${last_search_engine_page_counter_file}
mv ${local_url_time_series_search_log_counter_file} ${last_url_time_series_search_log_counter_file}
#mv ${local_url_time_series_counter_file} ${last_url_time_series_counter_file}

## 数字格式化:
uniq=`echo "scale=3;$delta_uniq_urls/1000" | bc`
dup=`echo "scale=3;$delta_dup_urls/1000" | bc`
hub=`echo "scale=3;$delta_hub_page_urls/1000" | bc`
hub2=`echo "scale=3;$delta_hub_page_urls2/1000" | bc`
hub3=`echo "scale=3;$delta_hub_page_urls3/1000" | bc`
rq=`echo "scale=3;$delta_search_engine_page_urls/1000" | bc`
sl=`echo "scale=3;$delta_url_time_series_search_log_urls/1000" | bc`
ts_uv=`echo "scale=3;$delta_url_time_series_urls/1000" | bc`

msg="realtime_crawler[issue_selected_urls_stat]: Uniq url: ${uniq}k, Duplicate url: ${dup}k, Hub page0: ${hub3}k, Hub page1: ${hub}k, Hub Page2: ${hub2}k, Realtime query: ${rq}k, Search log: ${sl}k, Time series UV: ${ts_uv}k"
SendTxtReport "$msg" "$receiver" 

exit 0
