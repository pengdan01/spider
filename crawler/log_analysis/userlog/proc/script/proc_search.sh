#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh

set -u

DEFINE_string  'output_search_path'  "/tmp/$USER/$$.`date +%d%H%M%S`"  'output hdfs path'     ''
DEFINE_string  'output_title_path'  "/tmp/$USER/$$.`date +%d%H%M%S`"  'output hdfs path'     ''
DEFINE_string  'input_path'   '/data/userlog/token_url'         'input hdfs path'      ''

DEFINE_string  'date'         "`date -d '1 day ago' +%Y%m%d`"   'the date of log to be processed. format is YYYYMMDD'
DEFINE_string  'alert_mail'   'zhengying@oneboxtech.com'        'email to receive alert'

FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# 检查参数
${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_input_path}
[[ $? -ne 0 ]]  && SendAlertViaMailAndDie "input path is NULL"

${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_search_path}
[[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop fs -mkdir ${FLAGS_output_search_path}

${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_title_path}
[[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop fs -mkdir ${FLAGS_output_title_path}

raw_input="${FLAGS_input_path}/${FLAGS_date}"
# 实际上给最后一轮的数据还有一些中间数据, 大于 raw_input 的大小
final_tasks=$(echo $(HadoopDus $raw_input) | awk '{print int($0/2000000000)+1}')

mapper_cmd="./search_plus.opt"
reducer_cmd="./search_plus.opt"
output="/tmp/${USER}/$$.`date +%m%d%H%M%S`.st"
${HADOOP_FS_CMD} -rmr $output
./mapreduce/submit.py \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_input $raw_input \
  --mr_input_format streaming_text \
  --mr_output_format streaming_text \
  --mr_output $output \
  --mr_reduce_tasks $final_tasks \
  --mr_job_name new_search_log.$USER.`date +%d%H%M` \
  --mr_disable_input_suffix_check \
  --mr_ignore_bad_compress --mr_min_split_size 2000 --mr_compress_st_output --mr_multi_output

[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mapred task failed" "${FLAGS_alert_mail}"

# output pv log
final_output="${FLAGS_output_search_path}/${FLAGS_date}_00_00.st" 
$HADOOP_HOME/bin/hadoop fs -rmr $final_output
$HADOOP_HOME/bin/hadoop fs -mkdir $final_output
$HADOOP_HOME/bin/hadoop fs -mv $output/searchlog* $final_output
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "hadoop mv failed" "${FLAGS_alert_mail}"
$HADOOP_HOME/bin/hadoop fs -mv $output/_abnormal_data.kv $final_output

# output title. 
# detect if dest dir exists
final_output="${FLAGS_output_title_path}/${FLAGS_date}_00_00.st"
${HADOOP_HOME}/bin/hadoop fs -test -d ${final_output}
if [ $? -ne 0 ]; then
  $HADOOP_HOME/bin/hadoop fs -mkdir $final_output
fi
$HADOOP_HOME/bin/hadoop fs -rm $final_output/title_search*
$HADOOP_HOME/bin/hadoop fs -mv $output/title_search* $final_output
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "hadoop mv failed" "${FLAGS_alert_mail}"

${HADOOP_HOME}/bin/hadoop fs -rmr $output
