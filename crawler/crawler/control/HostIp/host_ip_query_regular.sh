#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh
set -u
set +x

print_help=false
if [[ $# -eq 0 ]];then
  print_help=true
fi

timestamp=$(date +%s_%N)_$$

# 输入，输出目录
DEFINE_string 'site_uv_daily_root' '/app/feature/tasks/page_analyze/data/site_uv/daily' 'HDFS 输入路径，必须是 streaming-text 格式' 'i'
DEFINE_string 'to_date' '20120424' '合并多天的 uv log, 直到 to_date 指定的日期为止, 包括该日期'
DEFINE_integer 'date_back_interval' 40 '往前回溯的天数, 0 表示不回溯, 必须不小于 0'

DEFINE_string 'dns_servers' '10.115.80.21,10.115.80.22,10.115.80.23,10.115.80.24,10.115.80.25' 'dns server 列表'
DEFINE_integer 'window_size' 200 '对每个 dns server 的查询并发度'
DEFINE_integer 'max_retry' 1 '对查询超时的 host 进行 retry 的最大次数'

DEFINE_string 'output_root' '/app/feature/tasks/page_analyze/data/host_ip/' 'HDFS 输出路径，和输入格式一致' 'o'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x
[[ -z "${FLAGS_site_uv_daily_root}" ]] && EchoAndDie "uv rank daily root path invalid"
is_valid_date ${FLAGS_to_date} || EchoAndDie "Invalid to_date"

tmp_dir=/tmp/${USER}/${HOSTNAME}.${timestamp}/

# 得到 host 集合
log_analysis/page_importance/site_uv/host_uv_multi_days.sh \
   --site_uv_daily_root ${FLAGS_site_uv_daily_root} \
   --to_date ${FLAGS_to_date} \
   --date_back_interval ${FLAGS_date_back_interval} \
   --output ${tmp_dir}
if [[ $? -ne 0 ]];then
  EchoAndDie "site gathering failed"
fi

host_file=./hosts.${timestamp}
${HADOOP_HOME}/bin/hadoop dfs -cat ${tmp_dir}/${FLAGS_to_date}_00_00.st/part* | \
   bash build_tools/mapreduce/lzo_decompress.sh > ${host_file}
if [[ $? -ne 0 ]];then
  EchoAndDie "get host file failed"
fi

# 查询 DNS Server
host_ip_file=./hosts_ip.${timestamp}
cat ${host_file} | util/misc/dns_query_demo/dns_host.opt \
  --dns_server ${FLAGS_dns_servers} \
  --window_size ${FLAGS_window_size} \
  --dns_outer_try_times ${FLAGS_max_retry} \
  --logbuflevel -1 > ${host_ip_file}
if [[ $? -ne 0 ]];then
  EchoAndDie "dns query failed"
fi

# 查询结果上传
${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_output_root}/${FLAGS_to_date}_00_00.st
${HADOOP_HOME}/bin/hadoop dfs -mkdir ${FLAGS_output_root}/${FLAGS_to_date}_00_00.st
${HADOOP_HOME}/bin/hadoop dfs -put ${host_ip_file} ${FLAGS_output_root}/${FLAGS_to_date}_00_00.st
if [[ $? -ne 0 ]];then
  EchoAndDie "upload dns query result failed"
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_dir}
rm -f ${host_file} ${host_ip_file}

exit 0
