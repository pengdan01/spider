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
DEFINE_string 'host_ip_db_root' '/app/feature/tasks/page_analyze/data/host_ip/' 'HDFS 输入路径，必须是 streaming-text 格式' 'd'
DEFINE_string 'hosts' '' 'HDFS 输入路径, 每一行是一个host' 'i'

DEFINE_string 'dns_servers' '10.115.80.21,10.115.80.22,10.115.80.23,10.115.80.24,10.115.80.25' 'dns server 列表'
DEFINE_integer 'window_size' 300 '对每个 dns server 的查询并发度'
DEFINE_integer 'max_retry' 1 '对查询超时的 host 进行 retry 的最大次数'

DEFINE_string 'hdfs_output' '' 'HDFS 输出路径，和输入格式一致'
DEFINE_string 'local_output' '' '本地输出路径'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x
[[ -z "${FLAGS_host_ip_db_root}" ]] && EchoAndDie "host ip db root path invalid"

tmp_dir=/tmp/${USER}/${HOSTNAME}.${timestamp}/

# 从 host ip db 中查询
outof_db=${tmp_dir}/outofdb.st
newest_host_ip_info=`${HADOOP_HOME}/bin/hadoop dfs -ls ${FLAGS_host_ip_db_root} | tail -1 | awk '{print $NF}'`
./util/mr/dict_lookup/dict_lookup.sh \
    --dict_path "${newest_host_ip_info}" \
    --data_path ${FLAGS_hosts} \
    --output ${outof_db} \
    --field_id 0 \
    --not_found_substitute ""
if [[ $? -ne 0 ]];then
  echo "join host ip failed" >&2
  exit 1
fi
${HADOOP_HOME}/bin/hadoop dfs -cat ${outof_db}/part-* > ./url_host_dns_0_${timestamp}
if [[ $? -ne 0 ]];then
  echo "Get url host file failed" >&2
  exit 1
fi

# 无法从 db 中查到的内容直接查询 dns
awk 'NF==1{print $1}' ./url_host_dns_0_${timestamp} > ./url_host_dns_miss_${timestamp}
awk 'NF>1{print $0}' ./url_host_dns_0_${timestamp} > ./url_host_dns_success_${timestamp}

# 查询 DNS Server
host_ip_file=./url_host_dns_supplement_${timestamp}
cat ./url_host_dns_miss_${timestamp} | util/misc/dns_query_demo/dns_host.opt \
  --dns_server ${FLAGS_dns_servers} \
  --window_size ${FLAGS_window_size} \
  --dns_outer_try_times ${FLAGS_max_retry} \
  --logbuflevel -1 > ./url_host_dns_supplement_${timestamp}
if [[ $? -ne 0 ]];then
  EchoAndDie "dns query failed"
fi
#cat ./url_host_dns_supplement_${timestamp} >> ./url_host_dns_success_${timestamp}
awk 'NF>1 {
  out=$1;
  for (i=2;i<=NF;i++) {
    out=out"\t"$i;
  }
  print out;
}' ./url_host_dns_supplement_${timestamp} ./url_host_dns_success_${timestamp} > ./tmp.${timestamp} && \
    mv tmp.${timestamp} ./url_host_dns_success_${timestamp}

if [[ ! -z ${FLAGS_hdfs_output} ]];then
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_hdfs_output}
  ${HADOOP_HOME}/bin/hadoop dfs -mkdir ${FLAGS_hdfs_output}
  ${HADOOP_HOME}/bin/hadoop dfs -put ./url_host_dns_success_${timestamp} ${FLAGS_hdfs_output}
  [[ $? -ne 0 ]] && EchoAndDie "put to ${FLAGS_hdfs_output} fail"
fi

if [[ ! -z ${FLAGS_local_output} ]];then
  cp ./url_host_dns_success_${timestamp} ${FLAGS_local_output}
  [[ $? -ne 0 ]] && EchoAndDie "copy to ${FLAGS_local_output} fail"
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_dir}
rm ./url_host_dns_*_${timestamp}

exit 0
