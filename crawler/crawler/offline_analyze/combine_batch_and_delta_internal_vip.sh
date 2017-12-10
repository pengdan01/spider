#!/bin/bash
set -u
format_shard_id=`seq -w ${shard_id} ${shard_num} | head -1`

batch_hdfs_dir=${batch_hdfs_root_dir_vip}/shard_${format_shard_id}
delta_hdfs_dir=${delta_hdfs_root_dir_vip}/shard_${format_shard_id}

trash_batch_dir=${trash_hdfs_root_dir}/batch/${merge_type}_vip/shard_${format_shard_id}
trash_delta_dir=${trash_hdfs_root_dir}/delta/${merge_type}_vip/shard_${format_shard_id}

batch_donefile=${batch_hdfs_dir}/donefile
delta_donelist=${delta_hdfs_dir}/donelist

batch_lock_file=${batch_hdfs_dir}/_LOCK_FILE

## 加锁协议 （检查目录下是否已经加锁，如果已经加锁，则退出; 反之，加锁
${HADOOP_HOME}/bin/hadoop fs -test -e ${batch_lock_file} 
if [ $? -eq 0 ]; then
  echo "Error, hdfs directory[${batch_hdfs_dir}] is locked by someone else, we should wait..."
  exit 1
else
  ${HADOOP_HOME}/bin/hadoop fs -touchz ${batch_lock_file} 
  if [ $? -ne 0 ]; then
    echo "Error, FAIL: hadoop fs -touchz ${batch_lock_file}"
    exit 1
  fi
fi
batch_tmp_donefile=${status_dir}/tmp_batch_donefile.${merge_type}.${format_shard_id}
delta_tmp_donelist=${status_dir}/tmp_delta_donelist.${merge_type}.${format_shard_id}
reducer_num_file=${status_dir}/tmp_reducer_num.${merge_type}.${format_shard_id}
last_batch_donelist_record=""
${HADOOP_HOME}/bin/hadoop fs -test -e ${batch_donefile}
if [ $? -ne 0 ]; then
  echo "WARNING, batch_donefile[${batch_donefile}] not exist, it shoule be the first "
  "to run combine for this shard, otherwise it is an error"
else
  rm -f ${batch_tmp_donefile} &>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -get ${batch_donefile} $batch_tmp_donefile
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -get ${batch_donefile} $batch_tmp_donefile"
    ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
    exit 1
  fi
  last_batch_donelist_record=`cat $batch_tmp_donefile | tail -1`
fi
## 获取 delta donelist
${HADOOP_HOME}/bin/hadoop fs -test -e ${delta_donelist}
if [ $? -ne 0 ]; then
  echo "Error, delta_donelist[${delta_donelist}] not exist"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
rm -f $delta_tmp_donelist &>/dev/null
${HADOOP_HOME}/bin/hadoop fs -get ${delta_donelist} $delta_tmp_donelist 
if [ $? -ne 0 ]; then
  echo "Error, Fail: /hadoop fs -get ${delta_donelist} $delta_tmp_donelist"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
## 检查 delta donelist 是否有更新, 如果没有更新，则退出，不进行 merge
begin_line=1
if [ -s ${status_dir}/${merge_type}.delta_donelist.${format_shard_id} ]; then
  diff ${status_dir}/${merge_type}.delta_donelist.${format_shard_id} $delta_tmp_donelist &>/dev/null
  if [ $? -eq 0 ]; then
    echo "Warning, delta donelist not updated and no new page/css/images need to merge"
    ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
    exit 0
  fi
  last_line=`tail -1 ${status_dir}/${merge_type}.delta_donelist.${format_shard_id}`
  begin_line=`cat $delta_tmp_donelist | grep -n "${last_line}" | awk -F':' '{print $1}'`
  if [ "${begin_line}" ]; then
    ((begin_line++))
  else
    begin_line=1
  fi
fi
last_batch_hdfs_dir=`echo ${last_batch_donelist_record} | awk '{print $1}'`
input=${last_batch_hdfs_dir}
## 从 delta donelist 获取需要 merge 的数据 hdfs directory
cnt=1
while read line
do
  if [ ${cnt} -lt ${begin_line} ]; then
    ((cnt++))
    continue
  fi
  [ ! "${line}" ] && continue
  tmp=`echo ${line} | awk '{print $1}'`
  part_num=`${HADOOP_HOME}/bin/hadoop fs -ls ${tmp} | wc -l`
  [ ${part_num} -eq 0 ] && continue
  if [ ! "${input}" ]; then
    input=${tmp}
  else
    input=${input},${tmp}
  fi
done < $delta_tmp_donelist

timestamp=`date "+%F-%H%M%S"`
output=${batch_hdfs_dir}/${timestamp}.hf
tmp_output=${batch_hdfs_dir}/_tmp_${timestamp}.hf
## Start combine batch and delta 
mapper_cmd=".build/opt/targets/crawler/offline_analyze/mr_combine_batch_delta_mapper --logtostderr"
reducer_cmd=".build/opt/targets/crawler/offline_analyze/mr_combine_batch_delta_reducer --logtostderr"
mr_input_parameter="--mr_input ${input} --mr_input_format hfile"
build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_slow_start_ratio 0.95 \
  --mr_cache_archives ${crawler_dict_data}#web\
  --mr_map_cmd ${mapper_cmd} --mr_reduce_cmd ${reducer_cmd} \
  ${mr_input_parameter} \
  --mr_output ${tmp_output} --mr_output_format hfile \
  --mr_min_split_size 1024 \
  --mr_reduce_capacity ${hadoop_reduce_capacity} --mr_map_capacity ${hadoop_map_capacity} \
  --mr_reduce_tasks ${hadoop_reduce_task_num} \
  --mr_job_name "crawler_combine_${merge_type}.${format_shard_id}_vip"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -mv ${tmp_output} ${output}
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -mv ${tmp_output} ${output}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
## 每一个目录中会添加一个隐藏文件 _REDUCE_NUM, 记录本次 hadoop 输出的 reduce task数
echo ${hadoop_reduce_task_num} > ${reducer_num_file}
${HADOOP_HOME}/bin/hadoop fs -put ${reducer_num_file} ${output}/_REDUCE_NUM
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -put ${reducer_num_file} ${output}/_REDUCE_NUM"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
rm -f ${reducer_num_file}
echo -e "${output}\t${timestamp}" > $batch_tmp_donefile
${HADOOP_HOME}/bin/hadoop fs -rm ${batch_donefile}
${HADOOP_HOME}/bin/hadoop fs -put $batch_tmp_donefile ${batch_donefile}
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -put $batch_tmp_donefile ${batch_donefile}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
  exit 1
fi
rm -f $batch_tmp_donefile

## 删除上一次历史数据, 这一步操作需要十分小心, 千万不要误删除
## 这里不删除, 而是 move 到 .Trash 目录
${HADOOP_HOME}/bin/hadoop fs -mv ${last_batch_hdfs_dir} ${trash_batch_dir}
if [ $? -ne 0 ]; then
  echo "Oh, Take it easy! But Failed in: hadoop fs -mv ${last_batch_hdfs_dir} ${trash_batch_dir}"
fi

## XXX(pengdan): 考虑到时效性需要使用增量数据, 这里暂且不删除
## 删除 Delta 目录下的已经 merge 到 batch 中的数据目录
#cnt=1
#while read line
#do
#  if [ ${cnt} -lt ${begin_line} ]; then
#    ((cnt++))
#    continue
#  fi
#  [ ! "${line}" ] && continue
#  input=`echo ${line} | awk '{print $1}'`
#  ${HADOOP_HOME}/bin/hadoop fs -mv ${input} ${trash_delta_dir} 2>/dev/null
#  if [ $? -ne 0 ]; then
#    echo "Oh, Take it easy! But Failed in: hadoop fs -mv ${input} ${trash_delta_dir}"
#  fi
#done < $delta_tmp_donelist
#mv $delta_tmp_donelist ${status_dir}/${merge_type}.delta_donelist.${format_shard_id}

## Release LOCK
${HADOOP_HOME}/bin/hadoop fs -rm ${batch_lock_file}
echo "Done for merge_type: ${merge_type}, shard_id: ${format_shard_id} !"
