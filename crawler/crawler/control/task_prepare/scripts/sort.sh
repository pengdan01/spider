#!/bin/bash
if [[ $# -ne 4 ]];then
  echo "Usage: ./sort.sh <input> <output> <reducer_num> <web_db>" >&2
  exit 1
fi

source ./shell/common.sh
set -u
set -x

# 只允许跑一天的数据

input=$1
output=$2
reducer_num=$3
web_db=$4

total_size=`HadoopDus "$input"`
expected_reducer_size=`expr $total_size / $reducer_num`
if [[ ${expected_reducer_size} < 1 ]];then
  EchoAndDie "reduce size must larger than zero"
fi

./util/mr/tera_sort/tera_sort.sh \
    --input=$input \
    --output=$output \
    --type=double \
    --field_id=0 \
    --average_input_record_length=200 \
    --sorting_reduce_bucket_size=$expected_reducer_size \
    --reverse
if [[ $? -ne 0 ]];then
  EchoAndDie "tera sort failed"
fi

# XXX & TODO (huangboxiang): 网页库归一化, 本次归一化, 再去重, 可是归一化只是内部使用
# 除最高频次的 url 外, 和整个网页库去重
if (( reducer_num > 1 ));then
  # 建立网页库集合
  input_size=`HadoopDus "${web_db}"`
  ((r_task_num=input_size/1024/1024/1024/2))
  if [ ${r_task_num} \< 1 ];then
    r_task_num=1
  fi
  tmp_dir=/tmp/${USER}/crawler_`date +%S_%N`_$$.st/
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_dir}
  build_tools/mapreduce/submit.py \
    --mr_map_cmd ./bin/web_page_urls \
                 --logtostderr \
    --mr_input ${web_db} \
    --mr_reduce_cmd ./bin/web_page_urls \
                    --logtostderr \
    --mr_output ${tmp_dir} \
    --mr_output_format streaming_text \
    --mr_reduce_tasks ${r_task_num} \
    --mr_min_split_size 4048 \
    --mr_job_name "web_db...."
    # --mr_cache_archives /wly/web.tar.gz#web \
  if [ $? -ne 0 ];then
    EchoAndDie "web db failed" >&2
  fi
  # 去重
  tmp1_dir=/tmp/${USER}/output1_`date +%S_%N`_$$.st/
  tmp2_dir=/tmp/${USER}/output2_`date +%S_%N`_$$.st/
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp1_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp2_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -mv ${output} ${tmp1_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -mkdir ${tmp2_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp1_dir}/part-00000 ${tmp2_dir}/
  ./util/mr/set_filter/set_filter.sh -d ${tmp_dir} -i ${tmp1_dir} -o ${output} --field_id 1 --filt_out_in_dict
  if [ $? -ne 0 ];then
    EchoAndDie "set filter failed" >&2
  fi
  ${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp2_dir}/part-00000 ${output}/xx
  # 去重后再排序
  total_size=`HadoopDus "$output"`
  expected_reducer_size=`expr $total_size / $reducer_num`
  if [[ ${expected_reducer_size} < 1 ]];then
    EchoAndDie "reduce size must larger than zero"
  fi
  ./util/mr/tera_sort/tera_sort.sh \
      --input=$output \
      --output=$output \
      --type=double \
      --field_id=0 \
      --average_input_record_length=200 \
      --sorting_reduce_bucket_size=$expected_reducer_size \
      --reverse
  if [[ $? -ne 0 ]];then
    EchoAndDie "tera sort failed"
  fi
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp1_dir}
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp2_dir}
fi

exit 0;
