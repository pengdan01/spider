#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 stat_conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
tmp_dir=${stat_status_dir}
[ ! -d ${tmp_dir} ] && mkdir -p ${tmp_dir}

if [ ! ${pvlog_root_dir} -o ! ${stat_output_root_dir} ]; then
  echo "Error, parameter pvlog_donelist or stat_donelist or stat_output_root_dir is empty"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -test -d ${pvlog_root_dir}
if [ $? -ne 0 ]; then
  echo "Error, Not found pvlog_donelist[${pvlog_root_dir}] in hadoop"
  exit 1
fi
## Get pvlog_donelist to local disk
local_pvlog_donelist=${tmp_dir}/pvlog_donelist
local_pvlog_donelist_tmp=${local_pvlog_donelist}.tmp
${HADOOP_HOME}/bin/hadoop fs -ls ${pvlog_root_dir} | grep -v 'items' | awk '{print $NF}' > ${local_pvlog_donelist_tmp} 
if [ $? -ne 0 ]; then
  echo "Error, hadoop ls ${pvlog_root_dir} fail"
  exit 1
fi
latest_dir=`tail -1 ${local_pvlog_donelist_tmp}`
last_handled_latest_dir=""
## Check pv log donelist is updated or not, if not update, we will exit with warning
if [ -e ${local_pvlog_donelist} ]; then
  last_handled_latest_dir=`tail -1 ${local_pvlog_donelist}`
  if [ "${last_handled_latest_dir}" = "${latest_dir}" ]; then
    echo "Warning, pv log donelist no update"
    exit 0
  fi
fi
## 计算输入
start_line=0
if [ "${last_handled_latest_dir}" ]; then
  start_line=`grep -n "${last_handled_latest_dir}" ${local_pvlog_donelist_tmp} | awk -F':' '{print $1}'`
  [ ! "${start_line}" ] && start_line=0
fi
((start_line++))
cnt=1
pv_log_cnt=0  ## 记录总共处理了多少天的日志
input=""
while read line
do
  if [ $cnt -ge ${start_line} -a "${line}" ]; then
    ((pv_log_cnt++))
    if [ ! "${input}" ]; then
      input=${line}
    else
      input=${input},${line}
    fi
  fi
  ((cnt++))
done < ${local_pvlog_donelist_tmp}

if [ ! "${input}" ]; then
  echo "No Input, pv log donelist is empty"
  exit 1
fi
## 检查压力统计的历史数据
${HADOOP_HOME}/bin/hadoop fs -test -e ${stat_donelist}
if [ $? -eq 0 ]; then  ## 存在, 获取上一次 压力统计的数据
  ${HADOOP_HOME}/bin/hadoop fs -get ${stat_donelist} ${tmp_dir}/stat_donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Error, Failed to hadoop fs -get ${stat_donelist}"
    exit 1
  fi
  last_stat_dir=`tail -1 ${tmp_dir}/stat_donelist.tmp | awk '{print $1}'`
  if [ "${last_stat_dir}" ]; then
    input=${input},${last_stat_dir}
  fi
  rm -f ${tmp_dir}/stat_donelist.tmp &>/dev/null
fi

timestamp=`date "+%F-%H%M%S"`
output="${stat_output_root_dir}/${timestamp}.st"
tmp_output="${stat_output_root_dir}/_tmp_${timestamp}.st"
mapper=".build/opt/targets/crawler/statistic/mr_stat_site_pv_mapper --url_field_id=0 --logtostderr"
reducer=".build/opt/targets/crawler/statistic/mr_stat_site_pv_reducer --pv_log_num=${pv_log_cnt} \
--logtostderr"
build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper} --mr_reduce_cmd ${reducer} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${tmp_output} --mr_output_format streaming_text \
  --mr_min_split_size 1024 --mr_reduce_capacity 1000 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks ${reduce_task_num} \
  --mr_job_name "crawler_statistics"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -mv ${tmp_output} ${output}
if [ $? -ne 0 ]; then
  echo "Error, hadoop fs -mv ${tmp_output} ${output} fail"
  exit 1
fi
## Done, Now append a record to stat_donelist 
new_record="${output}\t${timestamp}"
local_stat_donelist_tmp=${tmp_dir}/stat_donelist.tmp
rm -f ${local_stat_donelist_tmp} &>/dev/null
${HADOOP_HOME}/bin/hadoop fs -test -e ${stat_donelist}
if [ $? -eq 0 ]; then
  ${HADOOP_HOME}/bin/hadoop fs -get ${stat_donelist} ${local_stat_donelist_tmp}
  if [ $? -ne 0 ]; then
    echo "Error, fail: hadoop fs -get ${stat_donelist} ${local_stat_donelist_tmp}"
    exit 1
  fi
fi
echo -e ${new_record} >> ${local_stat_donelist_tmp}
${HADOOP_HOME}/bin/hadoop fs -rm ${stat_donelist} 2>/dev/null
${HADOOP_HOME}/bin/hadoop fs -put ${local_stat_donelist_tmp} ${stat_donelist}
if [ $? -ne 0 ]; then
  echo "Error, fail: hadoop fs -put ${local_stat_donelist_tmp} ${stat_donelist}"
  exit 1
fi
mv ${local_pvlog_donelist_tmp} ${local_pvlog_donelist}

echo "Done, stat succeed!"
exit 0
