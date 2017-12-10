#!/bin/bash
set -u
set -x

if [ $# -ne 4 ]; then
  echo "Usage: $0 <fetcher_root> <last_update_file> <shard_output_dir> <shard_num>"
  exit 1
fi

function in_arr()
{
  local item=$1
  local arr_file=$2
  while read each;do
    if [[ "${item}" == "${each}" ]];then
      return 0
    fi
  done < ${arr_file}
  return 1
}

timestamp=`date +%Y%m%d%s`_$$

fetcher_root=$1
last_update_file=$2
shard_output_dir=$3
shard_number_info=$4
hdfs_tmp_dir=/tmp/${USER}_crawler_${timestamp}

# 读取 last update file
# last update 文件格式:
# last updated donelist_item
[[ ! -f ${last_update_file} ]] && EchoAndDie "Invalid Last Update File"

# 检查 donelist 查看最新抓取的结果
# 把新增的结果填充 increment_arr
# 同时生成 临时的"last_update_file"
[[ ! -d ./tmp ]] && mkdir ./tmp
tmp_last_update_file=./tmp/${last_update_file}
cat /dev/null > ${tmp_last_update_file}

idx=0
for donelist in `${HADOOP_HOME}/bin/hadoop dfs -ls ${fetcher_root}/fetcher_*/output/p*/donelist | awk '{print $NF}'`;do
  rm -f local_done_list
  ${HADOOP_HOME}/bin/hadoop dfs -get ${donelist} local_done_list
  if [[ $? -ne 0 ]];then
    EchoAndDie "fail to get donelist file"
  fi
  first_line=0
  tac local_done_list > reverse_local_done_list
  while read line; do
    [[ ${first_line} == 0 ]] && echo "${line}" >> ${tmp_last_update_file}
    ((first_line++))
    in_arr "${line}" "${last_update_file}" && break;
    increment_arr[idx]=`echo "${line}" | awk '{print $1}'`
    ((idx++))
    echo ${idx}
  done < reverse_local_done_list
  rm -f reverse_local_done_list
  rm -f local_done_list
done
if [[ ${idx} == 0 ]];then
  echo "nothing new to process, exit normally"
  exit 0
fi

echo "start moving to shard ${increment_arr[*]}"
exit 0

# 获得 shard_number_info
shard_number=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_number_info} | grep "page_shard_num" | awk -F"=" '{print $2}'`
if [ $? -ne 0 ]; then
  echo "cat shard_number_info file ${shard_number_info} failed"
  exit 1
fi

to_shard_input_data_path="${increment_arr[*]}"
to_shard_output_data_path="${hdfs_tmp_dir}/output.hf"

${HADOOP_HOME}/bin/hadoop fs -rmr ${to_shard_output_data_path}
build_tools/mapreduce/submit.py \
    --mr_map_cmd ./output_to_shard --shard_num=${shard_number} \
    --mr_reduce_cmd ./output_to_shard --shard_num=${shard_number} \
    --mr_input ${to_shard_input_data_path}  \
    --mr_output ${to_shard_output_data_path} \
    --mr_reduce_tasks=50 \
    --mr_input_format sfile \
    --mr_output_format hfile \
    --mr_multi_output \
    --mr_job_name crawler_schedule_to_shard
if [ $? -ne 0 ]; then
  echo "run hadoop job crawler_schedule_to_shard failed"
  exit 1
fi

echo "task crawler_schedule_to_shard finished, start to move data to output directory..."

((tmp=shard_number-1))
for i in `seq 0 $tmp`;
do
  #查看 shard 是否存在 不存在则创建目录
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir} failed"
      exit 1
    fi
  fi
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}/page
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir}/page doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}/page
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir}/page failed"
      exit 1
    fi
  fi
  shard_id=`printf %.3d ${i}`
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}/page/shard_${shard_id}
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}/page/shard_${shard_id}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir}/page/shard_${shard_id} failed"
      exit 1
    fi
  fi
  target_output_path=${shard_output_dir}/page/shard_${shard_id}/${timestamp}.hf
  $HADOOP_HOME/bin/hadoop fs -test -d ${target_output_path}
  if [ $? -ne 0 ]; then
    echo "${target_output_path} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${target_output_path}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${target_output_path} failed"
      exit 1
    fi
  fi
  $HADOOP_HOME/bin/hadoop fs -mv ${to_shard_output_data_path}/shard-${i}* ${target_output_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mv ${to_shard_output_data_path}/shard-${i}-* to ${target_output_path} failed"
    exit 1
  fi
done

mv ${tmp_last_update_file} ${last_update_file}

echo "Done moving to shard"
exit 0

