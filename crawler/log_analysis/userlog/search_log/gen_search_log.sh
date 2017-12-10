#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh

set -u

# 脚本参数, 如果是在其他脚本中调用本脚本，请使用 --xxxx 的参数名。命令行可以使用短参数名
#
# 输入、输出目录
DEFINE_string  'output_path'  "/tmp/$USER/$$.`date +%d%H%M%S`"  'output hdfs path'     'o'
DEFINE_string  'input_path'   '/data/userlog/token_query'       'input hdfs path'      'i'

DEFINE_string  'md5_query_donelist' '/app/user_data/md5_query/done_list' 'md5-query map donelist' 'q'
DEFINE_string  'md5_url_donelist'   '/app/user_data/md5_url/done_list'   'md5-url map donelist'   'u'

DEFINE_string  'date'         "`date -d '1 day ago' +%Y%m%d`"   'the date of log to be processed. format is YYYYMMDD'
DEFINE_string  'alert_mail'   'zhengying@oneboxtech.com'        'email to receive alert'
#
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# 检查参数
${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_input_path}
[[ $? -ne 0 ]]  && SendAlertViaMailAndDie "input path is NULL"

${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_path}
[[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop fs -mkdir ${FLAGS_output_path}
${HADOOP_HOME}/bin/hadoop fs -test -d ${FLAGS_output_path}
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "output path is NULL, and failed to create one"

${HADOOP_HOME}/bin/hadoop fs -test -e ${FLAGS_md5_query_donelist}
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "md5_query done list is NULL"

${HADOOP_HOME}/bin/hadoop fs -test -e ${FLAGS_md5_url_donelist}
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "md5_url done list is NULL"

raw_input="${FLAGS_input_path}/${FLAGS_date}"
# 保持与后续时效性提升后目录命名上的一致, 加上 00:00
final_output="${FLAGS_output_path}/${FLAGS_date}_00_00.st"

PWD=`pwd`
local_tmp=`mktemp -d --tmpdir=$PWD`
hdfs_tmp="/tmp/$USER.$$.`date +%Y%m%d%H%M%S`"

# 检测 md5_query 和 md5_url 的 done list，下载到本机，保存成两个变量，供后续使用
# 不建议 bash 新人这么写, 使用 function 也可以殊途同归
for src in `echo -e "query\nurl"`; do
  eval hdfs_donelist='$'FLAGS_md5_${src}_donelist
  local_donelist="${local_tmp}/`basename $hdfs_donelist`"
  HadoopForceGet "$hdfs_donelist" "$local_donelist"
  [[ $? -ne 0 ]] && SendAlertViaMailAndDie "get donelist [${hdfs_donelist}] failed" "${FLAGS_alert_mail}"

  md5_path="`tail -1 $local_donelist | awk '{print $1;}'`"
  [[ $? -ne 0 ]] && SendAlertViaMailAndDie "get path from [${local_donelist}] failed" "${FLAGS_alert_mail}"

  # mapred 任务依赖路径名称来识别是哪种类型的输入
  # 使用 dirname 去掉路径最后一个部分可能是通配符的情况。
  # 检查前面路径是否相同的情况，禁止这样的输入路径
  dir_md5="`dirname $md5_path`"
  dir_query="`dirname $raw_input`"
  [[ "${dir_md5}" = "${dir_query}" ]] && SendAlertViaMailAndDie "${md5_path} and $raw_input cannot in same dir" "${FLAGS_alert_mail}"

  # we will have two variables:  md5_query_path,  md5_url_path
  eval md5_${src}_path=$md5_path
done

### Round 1, split query log, and replace url md5
file="search_r1.opt"
mapper_cmd="./$file --md5_url_path `dirname ${md5_url_path}` --search_log_path `dirname ${raw_input}`"
reducer_cmd="./$file -md5_url_path `dirname ${md5_url_path}` --search_log_path `dirname ${raw_input}`"
output_path="${hdfs_tmp}/round_1.st"
input_path="${md5_url_path},${raw_input}"
$HADOOP_HOME/bin/hadoop fs -rmr $output_path
./mapreduce/submit.py --mr_map_cmd $mapper_cmd --mr_reduce_cmd $reducer_cmd \
    --mr_input "$input_path" --mr_input_format streaming_text \
    --mr_output "$output_path" --mr_output_format streaming_text \
    --mr_reduce_tasks 3000 \
    --mr_slow_start_ratio 0.95 \
    --mr_job_name "${USER}.${file}.`date +%d%H%M`" --mr_disable_input_suffix_check --mr_ignore_bad_compress

[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mapred task failed" "${FLAGS_alert_mail}"

### Round 2, replace query md5
last_round_output="${output_path}"
file="search_r2.opt"
mapper_cmd="./$file"
reducer_cmd="./$file"
output_path="${hdfs_tmp}/round_2.st"
input_path="${last_round_output}"
$HADOOP_HOME/bin/hadoop fs -rmr $output_path
./mapreduce/submit.py --mr_map_cmd $mapper_cmd --mr_reduce_cmd $reducer_cmd \
    --mr_input "$input_path" --mr_input_format streaming_text \
    --mr_output "$output_path" --mr_output_format streaming_text \
    --mr_reduce_tasks 500 \
    --mr_slow_start_ratio 0.95 --mr_compress_st_output \
    --mr_job_name "${USER}.${file}.`date +%d%H%M`" --mr_disable_input_suffix_check 

[[ $? -ne 0 ]] && SendAlertViaMailAndDie "mapred task failed" "${FLAGS_alert_mail}"

$HADOOP_HOME/bin/hadoop fs -rmr $final_output
$HADOOP_HOME/bin/hadoop fs -mv $output_path $final_output
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "hadoop mv failed" "${FLAGS_alert_mail}"
output_path="${final_output}"

# 收集前两轮的 error 输出，放到最终的 ouput 目录中
for i in `seq 1 1`; do
  ${HADOOP_HOME}/bin/hadoop fs -test -d "${hdfs_tmp}/round_${i}.st/_abnormal_data.kv"
  if [ $? -eq 0 ]; then
    # 目录名前加 _ ，这样就不妨碍 ${final_output} 整个目录作为其他 mapred 任务的输出，
    # 如果不加 _ , 由于存在目录嵌套，会导致以${final_output} 为源的任务失败
    ${HADOOP_HOME}/bin/hadoop fs -rmr "${final_output}/_r${i}_abnormal_data.kv"
    ${HADOOP_HOME}/bin/hadoop fs -mv "${hdfs_tmp}/round_${i}.st/_abnormal_data.kv" \
                                     "${final_output}/_r${i}_abnormal_data.kv"
    [[ $? -ne 0 ]] && SendAlertViaMail \
      "cp [${hdfs_tmp}/round_${i}.st/_abnormal_data.kv] to [${final_output}/_r${i}_abnormal_data.kv] failed" "${FLAGS_alert_mail}"
  fi
done

# update donelist
hdfs_donelist="${FLAGS_output_path}/done_list"
local_donelist="${local_tmp}/`basename $hdfs_donelist`"
rm ${local_donelist}
${HADOOP_HOME}/bin/hadoop fs -test -e $hdfs_donelist
if [ $? -eq 0 ] ; then
  HadoopForceGet "$hdfs_donelist" "$local_donelist"
  [[ $? -ne 0 ]] && SendAlertViaMailAndDie "get donelist from [${hdfs_donelist}] failed" "${FLAGS_alert_mail}"
fi
norm_output="`norm_path ${final_output}`"
echo -e "${norm_output}\t${FLAGS_date}0000\t${FLAGS_date}2359\t`date +%Y%m%d%H%M%S`" >> $local_donelist
HadoopForcePutFileToFile $local_donelist $hdfs_donelist
[[ $? -ne 0 ]] && SendAlertViaMailAndDie "put [$local_donelist]->[$hdfs_donelist] fail" "${FLAGS_alert_mail}"

rm -rf $local_tmp
${HADOOP_HOME}/bin/hadoop fs -rmr $hdfs_tmp

exit 0
