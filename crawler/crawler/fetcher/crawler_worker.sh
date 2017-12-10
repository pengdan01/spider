#!/bin/bash
set -u

[ $# -ne 2 ] && echo "Usage: $0 conf_file crawler_worker_id" && exit 1
conf_file=$1
crawler_worker_id=$2
[ ! -e ${conf_file} ]  && echo "Conf file[${conf_file}] not found" && exit 1

shell_file=shell/common.sh
[ ! -e ${shell_file} ] && echo "Error, shell_file[${shell_file}] not find" && exit 1

source ${shell_file}
source ${conf_file}

[ ! -d "${status_dir}" ] && mkdir -p "${status_dir}"

## 检查 文件锁 是否存在 并加锁 (每一个 worker 加 一个锁)
${HADOOP_HOME}/bin/hadoop fs -test -e ${lock_hdfs_file}.${crawler_worker_id}
## 任务还在运行中
if [ $? -eq 0 ]; then
  err_msg="Error(fetcher), fetcher is running, as find lock file: ${lock_hdfs_file}.${crawler_worker_id}" 
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi
## 上一次任务正常结束, 本次任务可以正常开始, 先加锁
${HADOOP_HOME}/bin/hadoop fs -touchz ${lock_hdfs_file}.${crawler_worker_id}
if [ $? -ne 0 ]; then
  err_msg="Fail(fetcher), hadoop fs -touchz ${lock_hdfs_file}.${crawler_worker_id}" 
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi

## 当 worker 异常退出时，该目录下存放大量临时文件
[ -d ./tmp_${crawler_worker_id} ] && rm -rf ./tmp_${crawler_worker_id} 
[ ! -d "${fetcher_log_dir}" ] && mkdir -p "${fetcher_log_dir}"
#.build/opt/targets/crawler/fetcher/crawler_fetcher --dns_servers=${dns_servers} \
.build/diag_dbg/targets/crawler/fetcher/crawler_fetcher --dns_servers=${dns_servers} \
  --status_dir=${status_dir} \
  --hdfs_working_dir=${crawler_hdfs_working_dir} \
  --thread_num=${fetcher_thread_num} \
  --ip_range_data_file="${ip_range_data_file}" \
  --crawler_worker_id=${crawler_worker_id} \
  --expose_counters_on_port=${counter_expose_port} \
  --working_dir="./tmp_${crawler_worker_id}/saver" \
  --max_concurrent_access_fetcher=3 \
  --max_concurrent_site_access=5 --max_default_qps=2 --max_fail_cnt=10000 \
  --switch_crawl_whole_image=false \
  --switch_filter_rules=true --max_http_session_time=200 \
  --max_follow_count=32 --debug_mode=0
## 正常情况下, 本脚本将一直 hold 到 此处. 因为 crawler_fetcher 会永远运行, 不会退出.
## 异常情况下, 返回非 0
if [ $? -ne 0 ]; then
  ## TODO(pengdan): 上传已经下载到本地的文件到 hadoop 上
  err_msg="Fail(fetcher), crawler_fetcher abort"
  echo ${err_msg} 
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}.${crawler_worker_id} 
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}.${crawler_worker_id} 
echo "Done!" && exit 0
