#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 hub_page_digt_conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
status_dir=crawler_status/hub_page_digt/
[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

if [ ! "${pvlog_donelist}" ]; then
  echo "Error, var pvlog_donelist is empty string" 
  exit 1
fi

${HADOOP_HOME}/bin/hadoop fs -test -e ${pvlog_donelist}
if [ $? -ne 0 ]; then
  echo "Error, Not found pvlog_donelist[${pvlog_donelist}] in hadoop"
  exit 1
fi
rm -f ${status_dir}/pvlog_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -get ${pvlog_donelist} ${status_dir}/pvlog_donelist.tmp
if [ $? -ne 0 ]; then
  echo "Error, hadoop get pvlog_donelist[${pvlog_donelist}] fail"
  exit 1
fi

## 任务是否已经处理过，即：pvlog_donelist 没有更新
if [ -e ${status_dir}/pvlog_donelist ]; then
  diff ${status_dir}/pvlog_donelist ${status_dir}/pvlog_donelist.tmp
  if [ $? -eq 0 ]; then
    echo "Warning, pvlog_donelist not updated and no new task need to process"
    exit 0
  fi
fi

## 从 pvlog_donelist 中获取最后一条记录，获取最新的 pv log 的 hdfs 路径
pvlog_last_record=`tail -1 ${status_dir}/pvlog_donelist.tmp`
if [ ! "${pvlog_last_record}" ]; then
  echo "Error, hdfs file pvlog_donelist has nothing, get input data fail"
  exit 1
fi
input=`echo ${pvlog_last_record} | awk '{print $1}'`

## for debug
#input=""

timestamp=`date "+%F-%H%M%S"`
output="${hdfs_tmp_output_root_dir}/${timestamp}.st"

mapper_cmd=".build/opt/targets/crawler/tools/hub_page_dig/mr_hub_page_dig_mapper \
 --logtostderr"
reducer_cmd=".build/opt/targets/crawler/tools/hub_page_dig/mr_hub_page_dig_reducer"

build_tools/mapreduce/submit.py \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 512 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 10 \
  --mr_job_name "hub_page_digt"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
## Sort in Reverse of Number (top N)
${HADOOP_HOME}/bin/hadoop fs -cat ${output}/part* > ${status_dir}/${hub_page_filename}.tmp
if [ $? -ne 0 ]; then
  echo "Error, fail: hadoop fs -cat ${output}/part* > ${status_dir}/${hub_page_filename}.tmp"
  exit 1
fi

sort -r -n -k 2 ${status_dir}/${hub_page_filename}.tmp > ${status_dir}/${hub_page_filename}
if [ $? -ne 0 ]; then
  echo "Sort fail"
  exit 1
fi
rm -f ${status_dir}/${hub_page_filename}.tmp

## Put local final data file to hadoop
final_output=${hdfs_final_output_root_dir}/${timestamp}
${HADOOP_HOME}/bin/hadoop fs -mkdir ${final_output}
${HADOOP_HOME}/bin/hadoop fs -put ${status_dir}/${hub_page_filename} ${final_output}
if [ $? -ne 0 ]; then
  echo "Error, FAIL: hadoop fs -put ${status_dir}/${hub_page_filename} ${final_output}"
  exit 1
fi
mv ${status_dir}/pvlog_donelist.tmp ${status_dir}/pvlog_donelist

echo "Done, Result put to hdfs: ${final_output}"

exit 0
