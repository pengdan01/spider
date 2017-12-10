#!/bin/bash
set -u

## 将VIP 增量目录某个 shard 的 page / css / image 等文件和对应的 batch 目录下的
## page / css / image 进行 merge
if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 combine_batch_and_delta_vip.conf"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
status_dir=~/crawler_status/combine_batch_delta_vip
[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

if [ ! "${shard_num_info_file_vip}" -o ! "${shard_id}" -o ! "${merge_type}" ]; then
  echo "Error, var shard_num_info_file_vip/shard_id/merge_type is empty"
  exit 1
fi
if [ "${merge_type}" != "page" -a "${merge_type}" != "css" -a "${merge_type}" != "image" ]; then
  echo "Error, value of merge_tye should be page,css or image, other value is invalid."
  exit 1
fi
## 检查爬虫数据词典文件是否存在
${HADOOP_HOME}/bin/hadoop fs -test -e ${crawler_dict_data}
if [ $? -ne 0 ]; then
  echo "Error, Not found crawler dict data[${crawler_dict_data}] in hadoop"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -test -e ${shard_num_info_file_vip}
if [ $? -ne 0 ]; then
  echo "Error, Not found shard_num_info_file_vip[${shard_num_info_file_vip}] in hadoop"
  exit 1
fi
## 获取 shard num
shard_num=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_file_vip} | grep "^${merge_type}_shard_num=" \
| awk -F'=' '{print $2}'`
if [ ! "${shard_num}" ]; then
  echo "Error, Failed to get shard_num from hdfs file ${shard_num_info_file_vip}"
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
  if [ ${start_shard_id} -lt 0 -o ${start_shard_id} -ge ${end_shard_id} -o \
    ${end_shard_id} -gt ${shard_num} ]; then
    echo "shard_id not valid, it should [0,${end_shard_id}]"
    exit 1
  fi
else
  start_shard_id=`echo ${shard_id} | awk -F',' '{print $1}'`
  ((end_shard_id = start_shard_id + 1))
fi
((--end_shard_id))
for i in `seq ${start_shard_id} ${end_shard_id}`
do
  shard_id=$i
  source crawler/offline_analyze/combine_batch_and_delta_internal_vip.sh
done

echo "Done!"
exit 0
