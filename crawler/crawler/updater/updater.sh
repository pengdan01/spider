#!/bin/bash
set -u

## 仅限于对网页库进行更新

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 updater.conf"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
status_dir=~/crawler_status/updater
[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

if [ ! "${shard_num_info_hdfs_file}" -o ! "${shard_id}" -o ! "${link_merge_donelist}" -o \
     ! "${updater_root_dir}" -o ! "${hadoop_reduce_task_num}" ]; then
  echo "Error, var shard_num_file/shard_id/link_merge_donelist/updater_root_dir/reduce_task_num is empty"
  exit 1
fi
## 获取 Linkbase 路径
${HADOOP_HOME}/bin/hadoop fs -test -e ${link_merge_donelist}
if [ $? -ne 0 ]; then
  echo "Error, Not found link_merge_donelist[${link_merge_donelist}] in hadoop"
  exit 1
fi
linkbase=`${HADOOP_HOME}/bin/hadoop fs -cat ${link_merge_donelist} | tail -1 | awk '{print $1}'`
if [ ! "${linkbase}" ]; then
  echo "WARNING, No Pages need to update, as linkbase dir is empty."
  exit 0
fi

${HADOOP_HOME}/bin/hadoop fs -test -e ${shard_num_info_hdfs_file}
if [ $? -ne 0 ]; then
  echo "Error, Not found shard_num_info_hdfs_file[${shard_num_info_hdfs_file}] in hadoop"
  exit 1
fi
## 获取 page shard num
shard_num=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_hdfs_file} | grep "^page_shard_num=" \
| awk -F'=' '{print $2}'`
if [ ! "${shard_num}" ]; then
  echo "Error, Failed to get shard_num from hdfs file ${shard_num_info_hdfs_file}"
  exit 1
fi
## 检查 shard id 是否有效： [0, shard_num)
field_num=`echo ${shard_id} | awk -F',' '{print NF}'`
if [ $field_num -eq 2 ]; then
  start_shard_id=`echo ${shard_id} | awk -F',' '{print $1}'`
  end_shard_id=`echo ${shard_id} | awk -F',' '{print $2}'`
  if [ ! "${start_shard_id}" -o ! "${end_shard_id}" ]; then
    echo "Error, Invalid shard_id parameter"
    exit 1
  fi
  if [ ${start_shard_id} -lt 0 -o ${start_shard_id} -gt ${end_shard_id} -o \
    ${end_shard_id} -gt ${shard_num} ]; then
    echo "shard_id value not valid, should [0,${end_shard_id}]"
    exit 1
  fi
else
  start_shard_id=`echo ${shard_id} | awk -F',' '{print $1}'`
  ((end_shard_id = start_shard_id + 1))
fi

## 从 Linkbase 中出该 shard 区间中需要更新的网页
input=${linkbase}
timestamp=`date "+%F-%H%M%S"`
output=${updater_root_dir}/${timestamp}.st
mapper=".build/opt/targets/crawler/updater/mr_updater_mapper --start_shard_id=${start_shard_id} \
--end_shard_id=${end_shard_id} --shard_num=${shard_num}"
reducer=".build/opt/targets/crawler/updater/mr_updater_reducer"
mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper} --mr_reduce_cmd ${reducer} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${output}.tmp.st --mr_output_format streaming_text \
  --mr_min_split_size 1024 \
  --mr_reduce_capacity 1000 --mr_map_capacity 1000 \
  --mr_reduce_tasks ${hadoop_reduce_task_num} \
  --mr_job_name "update_page_shard.${start_shard_id}.${end_shard_id}"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -mv ${output}.tmp.st ${output}
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -mv ${output}.tmp.st ${output}"
  exit 1
fi
## Append donelist
${HADOOP_HOME}/bin/hadoop fs -test -e ${updater_donelist}
if [ $? -eq 0 ]; then
  rm -f updater_donelist.tmp &>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -get ${updater_donelist} updater_donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -get ${updater_donelist} updater_donelist.tmp"
    exit 1
  fi
fi
echo -e "${output}\t${timestamp}" >> updater_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -rm ${updater_donelist} 
${HADOOP_HOME}/bin/hadoop fs -put updater_donelist.tmp ${updater_donelist} 
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -put updater_donelist.tmp ${updater_donelist}"
  exit 1
fi

echo "Done!"
exit 0
