#!/bin/bash
if [[ $# -ne 8 ]];then
  echo "Usage: ./uv_data.sh <input> <output> <pv_log_date> <reducer_num> <pvlogroot> <searchlogroot> <fetchedlogroot> <use_faildata_date>" >&2
  exit 1
fi
source ./shell/common.sh
set -u
set -x

# 运行指定日期的 pv 日志
input=$1
output=$2
pv_date=$3
reducer_num=$4
pv_log_root=$5
search_log_root=$6
fetched_log_root=$7
use_faildata_date=$8

# use PvLog, SearchLog, FetchedLog to generate uv data
timestamp=$(date +%s_%N)_$$
tmp_output=/tmp/${USER}_${HOSTNAME}/${timestamp}
$HADOOP_HOME/bin/hadoop fs -rmr ${tmp_output}/uvout.st
./build_tools/mapreduce/submit.py \
    --mr_map_cmd ./bin/uv_data \
      --logtostderr=1 \
      --pv_log_prefix=${pv_log_root} \
      --search_log_prefix=${search_log_root} \
      --fetched_log_prefix=${fetched_log_root} \
      --use_faildata_date=${use_faildata_date} \
      --max_try_times=3 \
    --mr_reduce_cmd ./bin/uv_data \
      --logtostderr=1 \
      --uv_lowerbound=1.99 \
    --mr_input ${input} \
    --mr_output ${tmp_output}/uvout.st \
    --mr_reduce_tasks ${reducer_num} \
    --mr_cache_archives /wly/web.tar.gz#web \
    --mr_output_counters=./uv_selected_stat_file.${pv_date} \
    --mr_job_name "UV_DATA_ON_${pv_date}"
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to compute UV data"
fi

# # url normalization
# $HADOOP_HOME/bin/hadoop fs -rmr ${tmp_output}/norm.st
# ./build_tools/mapreduce/submit.py \
#     --mr_map_cmd ./bin/norm_for_fetcher \
#       --logtostderr=1 \
#     --mr_reduce_cmd ./bin/norm_for_fetcher \
#       --logtostderr=1 \
#     --mr_input ${tmp_output}/uvout.st \
#     --mr_output ${tmp_output}/norm.st \
#     --mr_reduce_tasks ${reducer_num} \
#     --mr_cache_archives /wly/web.tar.gz#web \
#     --mr_job_name "UV_NORM_ON_${pv_date}"
# if [ $? -ne 0 ]; then
#   EchoAndDie "[CRAWLER_ERROR] Failed to norm UV data"
# fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${output}
${HADOOP_HOME}/bin/hadoop dfs -test -e `dirname ${output}`
if [[ $? -ne 0 ]];then
  ${HADOOP_HOME}/bin/hadoop dfs -mkdir `dirname ${output}`
fi
# ${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp_output}/norm.st ${output}
${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp_output}/uvout.st ${output}
if [[ $? -ne 0 ]];then
  EchoAndDie "fail to mv to ${output}"
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_output}

exit 0
