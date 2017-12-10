#!/bin/bash
if [[ $# -ne 2 ]];then
  echo "Usage: ./cut.sh <input> <output>" >&2
  exit 1
fi

source ./shell/common.sh
set -u
set -x

input=$1
output=$2
# TLD_DATA=/user/yanglei/tld.dat
timestamp=$(date +%s_%N)_$$
tmp_output=/tmp/${USER}_${HOSTNAME}/${timestamp}/tmp.st

${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_output}
./build_tools/mapreduce/submit.py \
  --mr_map_cmd ./bin/cut \
    --logtostderr=1 \
  --mr_input $input \
  --mr_output ${tmp_output} \
  --mr_reduce_tasks 0 \
  --mr_job_name="cut_data"
if [[ $? -ne 0 ]];then
  EchoAndDie "cut failed"
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${output}
${HADOOP_HOME}/bin/hadoop dfs -test -e `dirname ${output}`
if [[ $? -ne 0 ]];then
  ${HADOOP_HOME}/bin/hadoop dfs -mkdir `dirname ${output}`
fi
${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp_output} ${output}
if [[ $? -ne 0 ]];then
  EchoAndDie "fail to mv to ${output}"
fi

exit 0
