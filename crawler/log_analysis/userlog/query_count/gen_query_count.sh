#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh

set -u

# 脚本参数, 如果是在其他脚本中调用本脚本，请使用 --xxxx 的参数名。命令行可以使用短参数名
#
# 输入、输出目录
DEFINE_string  'output_path'  "/tmp/$USER/$$.`date +%d%H%M%S`" 'output hdfs path' 'o'
DEFINE_string  'pv_log'   '/app/user_data/pv_log'   'md5-url map donelist'   'i'

DEFINE_string  'date'         "`date -d '1 day ago' +%Y%m%d`" 'the date to process. format is YYYYMMDD'
DEFINE_string  'alert_mail'   'zhengying@oneboxtech.com'   'email to receive alert'
#
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

[[ -z ${FLAGS_pv_log} || -z ${FLAGS_output_path} ]] \
  && EchoAndDie "input or output path or program is NULL"

# 检查输出目录是否建立. 简单的检查即可
${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_path}
[[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop fs -mkdir ${FLAGS_output_path}

PWD=`pwd`

file="query_count.opt"
mapper_cmd="./$file"
reducer_cmd="./$file"
input_path="${FLAGS_pv_log}/${FLAGS_date}*/"
output_path="/tmp/$USER.$$.`date +%Y%m%d%H%M%S`.st"
final_output="${FLAGS_output_path}/${FLAGS_date}_00_00.st"  
$HADOOP_HOME/bin/hadoop fs -rmr $output_path
./mapreduce/submit.py --mr_map_cmd $mapper_cmd --mr_reduce_cmd $reducer_cmd \
    --mr_input "$input_path" --mr_input_format streaming_text \
    --mr_output "$output_path" --mr_output_format streaming_text \
    --mr_reduce_tasks 10 \
    --mr_slow_start_ratio 0.95 --mr_compress_st_output \
    --mr_job_name "${USER}.${file}.`date +%d%H%M`" --mr_disable_input_suffix_check

[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mapred task failed" "${FLAGS_alert_mail}"

$HADOOP_HOME/bin/hadoop fs -rmr $final_output
$HADOOP_HOME/bin/hadoop fs -mv $output_path $final_output
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mv output failed" "${FLAGS_alert_mail}"
exit 0

