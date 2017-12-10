#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 offline_site_stat_conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
tmp_dir=./tmp/tool_site_stat
[ ! -d ${tmp_dir} ] && mkdir -p ${tmp_dir}

if [ ! "${crawler_saver_donelist}" -o ! "${site_list}" ]; then
  echo "Error, var saver_donelist or site_list is empty string" 
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
## 从 saver_last_record 获取最新抓取的 link 路径，用作本次离线分析的输入
input=`echo ${saver_last_record} | awk '{print $2}'`

## for debug
#input=/user/pengdan/crawler/test_dir/link/2012-12-29-154250.st

timestamp=`date "+%F-%H%M%S"`
output="${hdfs_tmp_output_root_dir}/${timestamp}.st"

mapper_cmd=".build/opt/targets/crawler/tools/offline_site_stat/mr_offline_site_stat_mapper \
--site_list=${site_list} --logtostderr"
reducer_cmd=".build/opt/targets/crawler/tools/offline_site_stat/mr_offline_site_stat_reducer"

build_tools/mapreduce/submit.py \
  --mr_multi_output \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 512 \
  --mr_map_capacity ${hadoop_map_capacity} \
  --mr_reduce_tasks 10 \
  --mr_job_name "offline_site_stat_tool"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
## Multi Output
${HADOOP_HOME}/bin/hadoop fs -cat ${output}/specific_site* > ${tmp_dir}/specific_site_stat_result
if [ $? -ne 0 ]; then
  echo "Error, fail: hadoop fs -cat ${output}/specific_site* > ${tmp_dir}/specific_site_stat_result" 
  exit 1
fi
cat ${tmp_dir}/specific_site_stat_result

## Sort in Reverse of Number (top N)
${HADOOP_HOME}/bin/hadoop fs -cat ${output}/top_n* > ${tmp_dir}/top_n_stat_result.tmp
if [ $? -ne 0 ]; then
  echo "Error, fail: hadoop fs -cat ${output}/top_n* > ${tmp_dir}/top_n_stat_result.tmp"
  exit 1
fi

sort -r -n -k 2 ${tmp_dir}/top_n_stat_result.tmp > ${tmp_dir}/top_n_stat_result
if [ $? -ne 0 ]; then
  echo "Sort fail"
  exit 1
fi
rm -f ${tmp_dir}/top_n_stat_result.tmp

## Put local final data file to hadoop
final_output=${hdfs_final_output_root_dir}/${timestamp}
${HADOOP_HOME}/bin/hadoop fs -mkdir ${final_output}
${HADOOP_HOME}/bin/hadoop fs -put ${tmp_dir}/specific_site_stat_result ${final_output}
if [ $? -ne 0 ]; then
  echo "Error, FAIL: hadoop fs -put ${tmp_dir}/site_stat_result ${final_output}"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -put ${tmp_dir}/top_n_stat_result ${final_output}
if [ $? -ne 0 ]; then
  echo "Error, FAIL: hadoop fs -put ${tmp_dir}/top_n_stat_result ${final_output}"
  exit 1
fi

echo "Done, Result put to hdfs: ${final_output}"
exit 0
