#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 offline_analyze_conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
tmp_dir=~/crawler_status/offline_analyze
[ ! -d ${tmp_dir} ] && mkdir -p ${tmp_dir}

if [ ! "${crawler_saver_donelist}" -o ! "${newlink_output_root_dir}" -o \
     ! "${newlink_donelist}" -o ! "${anchor_output_root_dir}" -o \
     ! "${anchor_donelist}" ]; then
  echo "Error, var saver_donelist/newlink_output_root_dir/newlink_donelist/anchor_output_root_dir/anchor_donelist is empty string" 
  exit 1
fi

${HADOOP_HOME}/bin/hadoop fs -test -e ${crawler_saver_donelist}
if [ $? -ne 0 ]; then
  echo "Error, Not found crawler_saver_donelist[${crawler_saver_donelist}] in hadoop"
  exit 1
fi
rm -f ${tmp_dir}/saver_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -get ${crawler_saver_donelist} ${tmp_dir}/saver_donelist.tmp
if [ $? -ne 0 ]; then
  echo "Error, hadoop get crawler_saver_donelist[${crawler_saver_donelist}] fail"
  exit 1
fi

## 任务是否已经处理过，即：saver_donelist 没有更新
if [ -e ${tmp_dir}/saver_donelist ]; then
  diff ${tmp_dir}/saver_donelist ${tmp_dir}/saver_donelist.tmp
  if [ $? -eq 0 ]; then
    echo "Warning, saver donelist not updated and no new task need to process"
    exit 0
  fi
fi

## 从 saver_donelist 中获取最后一条记录，获取已经抓取的 page 的 hdfs 路径
saver_last_record=`tail -1 ${tmp_dir}/saver_donelist.tmp`
if [ ! "${saver_last_record}" ]; then
  echo "Error, hdfs file crawler_saver_donelist has nothing, get input data fail"
  exit 1
fi
## 从 saver_last_record 获取最新抓取的网页路径，用作本次离线分析的输入
input=`echo ${saver_last_record} | awk '{print $1}'`

## for debug
input="/user/pengdan/crawler/test_dir/page/2012-12-29-154250.hf"
#input="/user/pengdan/crawler/test_dir/css/2012-12-29-154250.hf"
#input=/user/pengdan/crawler/test_dir/image/2012-12-29-154250.hf

link_prefix="link"
anchor_prefix="anchor"

timestamp=`date "+%F-%H%M%S"`
newlink_output="${newlink_output_root_dir}/${timestamp}.st"
anchor_output="${anchor_output_root_dir}/${timestamp}.st"

mapper_cmd=".build/opt/targets/crawler/offline_analyze/mapper_offline_analyze --logtostderr"
reducer_cmd=".build/opt/targets/crawler/offline_analyze/reducer_offline_analyze --link_prefix=${link_prefix} \
--anchor_prefix=${anchor_prefix} --logtostderr"
mapper_file=.build/opt/targets/crawler/offline_analyze/mapper_offline_analyze
reducer_file=.build/opt/targets/crawler/offline_analyze/reducer_offline_analyze

build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper_cmd} --mr_reduce_cmd ${reducer_cmd} \
  --mr_input ${input} --mr_input_format hfile \
  --mr_output ${newlink_output} --mr_output_format streaming_text \
  --mr_min_split_size 512 --mr_multi_output \
  --mr_reduce_capacity ${hadoop_reduce_capacity} --mr_map_capacity ${hadoop_map_capacity} \
  --mr_reduce_tasks ${hadoop_reduce_task_num} \
  --mr_job_name "crawler_offline_analyze"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
## 检查是否有输出，即 hadoop 输出目录是否为空
link_part_num=`${HADOOP_HOME}/bin/hadoop fs -ls ${newlink_output}/${link_prefix}* 2>/dev/null | wc -l`
anchor_part_num=`${HADOOP_HOME}/bin/hadoop fs -ls ${newlink_output}/${anchor_prefix}* 2>/dev/null | wc -l`
((total_num = link_part_num + anchor_part_num))
if [ ${total_num} -eq 0 ]; then
  echo "Warning, hadoop output directory is empty(has no part output)"
  exit 0
fi

## 多路输出只是对每一路输出冠以不同文件名前缀，他们仍在同一个目录，需要由应用程序负责将各路输出
## 移到特定的目录
## mv anchor text to anchor_text hdfs output dir
${HADOOP_HOME}/bin/hadoop fs -test -d ${anchor_output} </dev/null &>/dev/null
if [ $? -ne 0 ]; then
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${anchor_output}
  if [ $? -ne 0 ]; then
    echo "Error, hadoop fs -mkdir ${anchor_output} fail"
    exit 1
  fi
else
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${anchor_output}/*
fi

${HADOOP_HOME}/bin/hadoop fs -mv ${newlink_output}/${anchor_prefix}* ${anchor_output}
if [ $? -ne 0 ]; then
  echo "Error, hadoop mv ${newlink_output}/${anchor_prefix}* ${anchor_output} fail"
  exit 1
fi

## Append anchor donelist
record="${anchor_output}\t${timestamp}"
local_anchor_donelist="${tmp_dir}/anchor_donelist.tmp"
${HADOOP_HOME}/bin/hadoop fs -test -e ${anchor_donelist}
if [ $? -eq 0 ]; then
  rm -f ${local_anchor_donelist}
  ${HADOOP_HOME}/bin/hadoop fs -get ${anchor_donelist} ${local_anchor_donelist}
  if [ $? -ne 0 ]; then
    echo "Error, hadoop fs -get ${anchor_donelist} ${local_anchor_donelist} fail"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rm ${anchor_donelist}
fi
echo -e ${record} >> ${local_anchor_donelist} 
${HADOOP_HOME}/bin/hadoop fs -put ${local_anchor_donelist} ${anchor_donelist} 
if [ $? -ne 0 ]; then
  echo "hadoop fs -put ${local_anchor_donelist} ${anchor_donelist} fail"
  exit 1
fi

## Now append a record to newlink_donelist 
record="${newlink_output}\t${timestamp}"
local_newlink_donelist="${tmp_dir}/newlink_donelist.tmp"
${HADOOP_HOME}/bin/hadoop fs -test -e ${newlink_donelist}
if [ $? -eq 0 ]; then
  rm -f ${local_newlink_donelist}
  ${HADOOP_HOME}/bin/hadoop fs -get ${newlink_donelist} ${local_newlink_donelist}
  if [ $? -ne 0 ]; then
    echo "Error, hadoop fs -get ${newlink_donelist} ${local_newlink_donelist} fail"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rm ${newlink_donelist}
fi
echo -e ${record} >> ${local_newlink_donelist} 
${HADOOP_HOME}/bin/hadoop fs -put ${local_newlink_donelist} ${newlink_donelist} 
if [ $? -ne 0 ]; then
  echo "Error, Failed to put ${local_newlink_donelist} ${newlink_donelist}"
  exit 1
fi

## After analyze all those pages, now we need mv those pages to /app/crawler/delta
## This work is done in scirpt: move_to_delta.sh

echo "Done!"
exit 0
