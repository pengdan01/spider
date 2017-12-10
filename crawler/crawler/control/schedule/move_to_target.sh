#! /bin/bash

set -u
if [ $# -ne 6 ]; then
  echo "Error: Usage: $0 fetcher根目录 fetcher_id 优先级 时间戳 输出目录 host_load文件地址"
  exit 1
fi

fetcher_input_path=$1
fetcher_i=$2
priority=$3
timestamp=$4
output_data_path=$5
host_load_control_path=$6

# 先获取目标路径地址 如果不存在则创建
fetcher_i_str=`printf %.3d ${fetcher_i}`
fetcher_i_path=${fetcher_input_path}/fetcher_${fetcher_i_str}
$HADOOP_HOME/bin/hadoop fs -test -d ${fetcher_i_path}
if [ $? -ne 0 ]; then
  echo "${fetcher_i_path} doesn't exist, will create it!"
  $HADOOP_HOME/bin/hadoop fs -mkdir ${fetcher_i_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mkdir ${fetcher_i_path} failed"
    exit 1
  fi
fi
fetcher_i_input_path=${fetcher_i_path}/input
$HADOOP_HOME/bin/hadoop fs -test -d ${fetcher_i_input_path}
if [ $? -ne 0 ]; then
  echo "${fetcher_i_input_path} doesn't exist, will create it!"
  $HADOOP_HOME/bin/hadoop fs -mkdir ${fetcher_i_input_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mkdir ${fetcher_i_input_path} failed"
    exit 1
  fi
fi
# 获取优先级目标路径地址 如果不存在则创建
priority_output_path=${fetcher_i_input_path}/p${priority}
$HADOOP_HOME/bin/hadoop fs -test -d ${priority_output_path}
if [ $? -ne 0 ]; then
  echo "${priority_output_path} doesn't exist, will create it!"
  $HADOOP_HOME/bin/hadoop fs -mkdir ${priority_output_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mkdir ${priority_output_path} failed"
    exit 1
  fi
fi
# 下载 donelist 如果不存在则本地创建一个
local_done_list_path="./${fetcher_i_str}.p${priority}.donelist.${timestamp}"
rm -rf ${local_done_list_path}
${HADOOP_HOME}/bin/hadoop fs -get ${priority_output_path}/donelist ${local_done_list_path}
if [ $? -ne 0 ]; then
  echo "donelist does't exist on hadoop, we will create it!"
  touch ${local_done_list_path}
fi
# 获得调度器输出的该fetcher的输入
fetcher_input_file_list=`$HADOOP_HOME/bin/hadoop fs -ls ${output_data_path} | awk -v fetcher_fid="^fetcher_${fetcher_i}_" -F"/" '{if($NF ~ fetcher_fid) {print $NF}}' | sort -t"_" -k4 -n -r`
for fetcher_input_file in ${fetcher_input_file_list}; do
  task_id=`echo $fetcher_input_file | awk -F"task_|-" '{print $2}'`
  task_reduce_id=`echo $fetcher_input_file | awk -F"task_|-" '{print $3}'`
  task_id_str=`printf %.3d ${task_id}`
  mv_src_path=${output_data_path}/${fetcher_input_file}
  mv_target_path=${priority_output_path}/${timestamp}_${task_id_str}_${task_reduce_id}
  ${HADOOP_HOME}/bin/hadoop fs -mv ${mv_src_path} ${mv_target_path}.url
  if [ $? -ne 0 ]; then
    echo "mv from $mv_src_path to $mv_target_path failed"
    exit 1
  fi
  # 上传 host load文件
  ${HADOOP_HOME}/bin/hadoop fs -put ${host_load_control_path} ${mv_target_path}.ip_load
  if [ $? -ne 0 ]; then
    echo "mv from $mv_src_path to $mv_target_path failed"
    exit 1
  fi
  #更新本地的donelist
  echo ${mv_target_path} >> ${local_done_list_path}
done
#上传 donelist 
${HADOOP_HOME}/bin/hadoop fs -put ${local_done_list_path} ${priority_output_path}/donelist.${timestamp}
if [ $? -ne 0 ]; then
  echo "upload donelist from ${local_done_list_path} to ${priority_output_path}/donelist.${timestamp} failed"
  exit 1
fi
# 查看原 donelist 是否存在 如果存在 删除原 donelist
${HADOOP_HOME}/bin/hadoop fs -test -e ${priority_output_path}/donelist
if [ $? -eq 0 ]; then
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${priority_output_path}/donelist &> /dev/null
  if [ $? -ne 0 ]; then
    echo "rmr donelist ${priority_output_path}/donelist failed"
    exit 1
  fi
fi
# mv donelist
${HADOOP_HOME}/bin/hadoop fs -mv ${priority_output_path}/donelist.${timestamp} ${priority_output_path}/donelist
if [ $? -ne 0 ]; then
  echo "move donelist from ${priority_output_path}/donelist.${timestamp} to ${priority_output_path}/donelist"
  exit 1
fi
# 删除本地 donelist
rm ${local_done_list_path}
if [ $? -ne 0 ]; then
  echo "rm local_done_list ${local_done_list_path} failed"
  exit 1
fi
