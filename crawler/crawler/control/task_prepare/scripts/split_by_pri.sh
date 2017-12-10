#!/bin/bash
if [[ $# -ne 7 ]];then
  echo "Usage: ./split_by_pri.sh <input> <output> <pv_log_date> <reducer_num> <pri_num> <split_type> <set_level_to>" >&2
  exit 1
fi

source ./shell/common.sh
set -u
set -x


input=$1
output=$2
pv_log_date=$3
reducer_num=$4
pri_num=$5

split_type=$6
set_level_to=$7

[[ ${pri_num} != 5 ]] && EchoAndDie "must have 5 levels priority"

pri_name=(5 4 3 2 1)
# pri_array=(1 12 188 199 200)

# 初始化为全 0
idx=0
while ((idx < pri_num)); do
  pri_array[${idx}]=0
  ((idx++))
done

if [[ ${split_type} == manual ]];then
  ((idx = pri_num - set_level_to))
  while ((idx < pri_num));do
    pri_array[${idx}]=${reducer_num}
    ((idx++))
  done
else
# 最高优先级和最低优先级保留
  ratio=(0 0.05 0.8 0.15 0)
  idx=1
  while ((idx < pri_num));do
    if ((idx >= pri_num-2));then
      pri_array[${idx}]=${reducer_num}
      ((idx++))
      continue
    fi
    ((last=idx-1))
    num=`awk -v r=${ratio[$idx]} -v l=${pri_array[$last]} -v total=${reducer_num} 'BEGIN{
      inc = total * r;
      if (inc < 1) {
        inc = 1;
      }
      num = l + inc;
      if (num > total) {
        num = total;
      }
      print int(num);
    }'`
    pri_array[${idx}]=${num}
    ((idx++))
  done
fi
echo "pri name: ${pri_name[@]}"
echo "pri array: ${pri_array[@]}"
[[ ${#pri_name[*]} -ne 5 ]] && EchoAndDie "must be 5"
[[ ${#pri_array[*]} -ne 5 ]] && EchoAndDie "must be 5"

timestamp=$(date +%s_%N)_$$
hdfs_tmp_dir=/tmp/${USER}_${HOSTNAME}/${timestamp}/

${HADOOP_HOME}/bin/hadoop fs -mkdir ${output}

# 创建各个优先级的任务路径
for (( i = 0; i < ${pri_num}; ++i)); do
  pri=${pri_name[$i]}
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${hdfs_tmp_dir}/${pv_log_date}_p${pri}.st
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${output}/${pv_log_date}_p${pri}.st
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${output}/${pv_log_date}_p${pri}.st
done;

# 按照文件名分优先级
tmp_control=0
for (( i = 0; i < $reducer_num; ++i)); do
  for (( j = 0; j < ${pri_num}; ++j)); do
   	if [ $i -lt ${pri_array[$j]} ]; then
   	  pri=${pri_name[$j]}
   	  filename=part-`printf "%05d" $i`
      {
   	    ${HADOOP_HOME}/bin/hadoop fs -mv ${input}/$filename ${hdfs_tmp_dir}/${pv_log_date}_p${pri}.st
      } &
      ((tmp_control++))
      if ((tmp_control > 10));then
        sleep 5
        tmp_control=0
      fi
   	  break
   	fi
  done
done
wait
if [[ $? -ne 0 ]];then
  EchoAndDie "mv from ${input}/ to ${hdfs_tmp_dir}/${pv_log_date}_pX.st/ fail"
fi

# 启动 cut 任务
for (( i = 0; i < ${pri_num}; ++i)); do
  pri=${pri_name[$i]}
  size=`HadoopDus "${hdfs_tmp_dir}/${pv_log_date}_p${pri}.st"`
  (( ${size} <= 0 )) && continue
  {
    bash -x cut.sh ${hdfs_tmp_dir}/${pv_log_date}_p${pri}.st ${output}/${pv_log_date}_p${pri}.st
  } &
  sleep 1
done;

wait
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to cut.";
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${hdfs_tmp_dir}

exit 0
