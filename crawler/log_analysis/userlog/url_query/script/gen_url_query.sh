#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh

set -u

# 脚本参数, 如果是在其他脚本中调用本脚本，请使用 --xxxx 的参数名。命令行可以使用短参数名
#
# 输入、输出目录
DEFINE_string  'output_path'  "/tmp/$USER/$$.`date +%d%H%M%S`" 'output hdfs path'     'o'
DEFINE_string  'input_path'   '' 'input hdfs path'         'i'
DEFINE_string  'date' "`date -d '1 day ago' +%Y%m%d`" 'the date of log to be processed. format is YYYYMMDD'
DEFINE_string  'alert_mail'   'zhengying@oneboxtech.com'   'email to receive alert'
DEFINE_integer 'reducer' '10' 'reducer number of final round'
#
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

[[ -z ${FLAGS_input_path} || -z ${FLAGS_output_path} ]] && EchoAndDie "input or output path is NULL"

# 检查输出目录是否建立. 简单的检查即可
${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_path}
[[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop fs -mkdir ${FLAGS_output_path}

PWD=`pwd`
local_tmp=`mktemp -d --tmpdir=$PWD`
hdfs_tmp="/tmp/$USER.$$.`date +%Y%m%d%H%M%S`.st"

hdfs_donelist="${FLAGS_output_path}/done_list"
local_donelist="${local_tmp}/`basename $hdfs_donelist`"
history_data=""

${HADOOP_HOME}/bin/hadoop fs -test -e $hdfs_donelist
if [ $? -eq 0 ] ; then
  HadoopForceGet "$hdfs_donelist" "$local_donelist"
  [[ $? -ne 0 ]] && SendAlertViaMailAndDie "get donelist from [${hdfs_donelist}] failed" "${FLAGS_alert_mail}"
  if [ -s $local_donelist ]; then
    history_data="`tail -1 $local_donelist | awk '{print $1;}'`"
    [[ $? -ne 0 ]] && SendAlertViaMailAndDie "get path from [${local_donelist}] failed" "${FLAGS_alert_mail}"
  fi
fi

# 保持与后续时效性提升后目录命名上的一致, 加上 00:00
output_path=$hdfs_tmp
# streaming_jar=$HADOOP_HOME/contrib/streaming/hadoop-0.20.1.12-streaming.jar
if [ "${history_data}" == "" ];then
  input_path="${FLAGS_input_path}/${FLAGS_date}_00_00.st"
  final_tasks=$(echo $(HadoopDus ${input_path}) | awk '{print int($0/5000000000)+1}')
else
  input_path="${FLAGS_input_path}/${FLAGS_date}_00_00.st,${history_data}"
  final_tasks=$(echo $(HadoopDus ${history_data}) | awk '{print int($0/1000000000)+1}')
fi

mapper_cmd="./url_query.opt"
reducer_cmd="./url_query.opt"

$HADOOP_HOME/bin/hadoop fs -rmr $output_path
./mapreduce/submit.py --mr_map_cmd $mapper_cmd --mr_reduce_cmd $reducer_cmd \
    --mr_input "$input_path" --mr_input_format streaming_text \
    --mr_output "$output_path" --mr_output_format streaming_text \
    --mr_reduce_tasks $final_tasks --mr_min_split_size 2000 \
    --mr_slow_start_ratio 0.95 \
    --mr_job_name "${USER}.url_query.`date +%d%H%M`" --mr_disable_input_suffix_check --mr_ignore_bad_compress

    #--mr_map_capacity 500 --mr_reduce_capacity 10 \--mr_compress_st_output \
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mapred task failed" "${FLAGS_alert_mail}"

output_path="${FLAGS_output_path}/${FLAGS_date}_00_00.st"  
$HADOOP_HOME/bin/hadoop fs -rmr $output_path
$HADOOP_HOME/bin/hadoop fs -mv $hdfs_tmp $output_path
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mv output failed" "${FLAGS_alert_mail}"

# update donelist and upload
# 数据目录  数据截止时间  数据生成时间
norm_output="`norm_path ${output_path}`"
echo -e "${norm_output}\t${FLAGS_date}2359\t`date +%Y%m%d%H%M%S`" >> $local_donelist
HadoopForcePutFileToFile $local_donelist $hdfs_donelist
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "put [$local_donelist]->[$hdfs_donelist] fail" "${FLAGS_alert_mail}"

rm -rf $local_tmp
exit 0

