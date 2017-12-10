#!/bin/bash
source ~/shell/common.sh
source conf.sh

# 运行指定日期的 pv 日志
INPUT=$1
OUTPUT=$2
JOB_NAME=site_data
$HADOOP_HOME/bin/hadoop fs -rmr $OUTPUT

$SUBMIT_SCRIPT \
    --mr_map_cmd ./bin/site_uv_mapper \
      --logtostderr=1 \
      --reducer_cnt=$REDUCER_NUM \
    --mr_reduce_cmd ./bin/site_uv_reducer \
      --logtostderr=1 \
    --mr_input $INPUT \
    --mr_output $OUTPUT \
    --mr_reduce_tasks $REDUCER_NUM \
    --mr_job_name $JOB_NAME \
    --mr_cache_archives ${PROG_DATA_PATH}/web.tar.gz#web 

if [ $? -ne 0 ]; then
  echo "$ERROR_STR Failed to compute UV data";
  exit -1;
fi
exit 0