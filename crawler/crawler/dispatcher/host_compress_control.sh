#!/bin/bash
set -u

if [ ! "${statistic_donelist}" ]; then
  err_msg="Error, parameter statistic_donelist in conf file is empty."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
## check donelist
${HADOOP_HOME}/bin/hadoop fs -test -e ${statistic_donelist}
if [ $? -ne 0 ] ; then
  err_msg="Error, selector donelist[${statistic_donelist}] not found in hadoop."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi

local_stat_donelist=${status_dir}/stat_donelist
rm -f ${local_stat_donelist}.tmp 2>/dev/null
${HADOOP_HOME}/bin/hadoop fs -get ${statistic_donelist} ${local_stat_donelist}.tmp
if [ $? -ne 0 ]; then
  err_msg="Fail, hadoop fs -get ${statistic_donelist} ${local_stat_donelist}.tmp"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
if [ -e ${local_stat_donelist} ]; then
  diff ${local_stat_donelist} ${local_stat_donelist}.tmp &>/dev/null
  if [ $? -eq 0 ]; then
    echo "Warning, stat donelist is the same as last time, so we will use the same donelist"
  fi
fi
stat_input=`tail -1 ${local_stat_donelist}.tmp | awk -F'\t' '{print $1}'`
if [ ! "${stat_input}" ]; then
  err_msg="Error, stat input is empty"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi

timestamp=`date "+%F-%H%M%S"`
## 自动计算 reduce task 个数, 每个文件 10MB
per_part_size=10*1024*1024
reduce_task_num=`echo "${total_file_size} / (${per_part_size})" | bc`
if [ ${reduce_task_num} -lt ${crawler_worker_num} ]; then
  reduce_task_num=${crawler_worker_num}
fi

input="${stat_input},${url_input_dir}"
output="${dispatcher_hdfs_root_dir}/control/${timestamp}.st"
mapper_cmd_str=".build/opt/targets/crawler/dispatcher/mapper_compress_control \
--max_currency_access_thread=3 --logtostderr"
reducer_cmd_str=".build/opt/targets/crawler/dispatcher/reducer_compress_control \
--max_currency_access_thread=3 --lamla=1.0 --ignore_overload_urls=false --logtostderr"
$HADOOP_HOME/bin/hadoop dfs -rmr ${output} &>/dev/null
build_tools/mapreduce/submit.py \
--mr_slow_start_ratio 0.95 \
--mr_map_cmd ${mapper_cmd_str} --mr_reduce_cmd ${reducer_cmd_str} \
--mr_input ${input} --mr_input_format streaming_text \
--mr_output "${output}" --mr_output_format streaming_text \
--mr_reduce_capacity 1000 --mr_map_capacity 1000 \
--mr_min_split_size 512 \
--mr_reduce_tasks ${reduce_task_num} \
--mr_job_name "wly-dispatcher-site-compress-control"
if [ $? -ne 0 ];then
  err_msg="Fail: hadoop job wly-dispatcher-site-compress-control"
  echo ${err_msg} 
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
echo "compress_control_output=${output}" > ${status_dir}/compress_control.info

url_input_dir=${output}
