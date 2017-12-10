#!/bin/bash
set -u
receiver=pengdan@oneboxtech.com
output_root_dir=/tmp/pengdan/check_big_site_valid_page

## State the input status of current day
## date -d "-1 day" +%Y%m%d
today=`date +%Y%m%d`
echo ${today}
input=general_crawler/fetcher_*/output/p*/${today}*.sf
#input=/user/crawler/general_crawler/fetcher_*/output/p*/20120611*.sf
timestamp=`date "+%F-%H%M%S"`
output="${output_root_dir}/${timestamp}.st"

shell_utility=~/control_workspace/shell
source ${shell_utility}/common.sh

mapper_cmd="/home/crawler/control_workspace/crawler_tools/check_page_content/mr_check_valid_page_mapper --bad_hash=badurl --logtostderr"
reducer_cmd="/home/crawler/control_workspace/crawler_tools/check_page_content/mr_check_valid_page_reducer --logtostderr"
~/control_workspace/build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_cache_archives /wly/web.tar.gz#web,/wly/nlp.tar.gz#nlp \
  --mr_cache_files /user/yuanjinhui/bad_hash#badurl \
  --mr_input ${input} --mr_input_format sfile \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 1024 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 10 \
  --mr_job_name "check_big_site_valid_page"
if [ $? -ne 0 ]; then
  err_msg="check_big_site_valid_page: error, hadoop job fail"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${receiver}"
  exit 1
fi
## Get the output to local
local_file=host_fetcher_status.txt
rm -f ${local_file}
${HADOOP_HOME}/bin/hadoop fs -getmerge ${output} ${local_file} 
if [ $? -ne 0 ]; then
  err_msg="check_big_site_valid_page: fail in hadoop fs -getmerge ${output} ${local_file}"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${receiver}"
  exit 1
fi
## 按照 已经抓取 url 的个数 降序排列
sort -n -k 2 -r ${local_file} > sort_by_big_site_desend.tmp
## 按照 抓取有效网页比例 升序排列
sort -n -k 4 ${local_file} > sort_by_valid_ratio.tmp

## Upload to hadoop
${HADOOP_HOME}/bin/hadoop fs -rm $output/sort_by_big_site_desend
${HADOOP_HOME}/bin/hadoop fs -put sort_by_big_site_desend.tmp  $output/sort_by_big_site_desend
if [ $? -ne 0 ]; then
  err_msg="check_big_site_valid_page: fail in hadoop fs -put sort_by_big_site_desend.tmp  $output/sort_by_big_site_desend"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${receiver}"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -rm $output/sort_by_valid_ratio
${HADOOP_HOME}/bin/hadoop fs -put sort_by_valid_ratio.tmp  $output/sort_by_valid_ratio
if [ $? -ne 0 ]; then
  err_msg="check_big_site_valid_page: fail in hadoop fs -put sort_by_valid_ratio.tmp  $output/sort_by_valid_ratio"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${receiver}"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -rm $output/part*

info="output: $output, "`head -10 sort_by_big_site_desend.tmp`
echo $info 

SendTxtReport "${info}" "${receiver}"

rm -f sort_by_big_site_desend.tmp sort_by_valid_ratio.tmp ${local_file}

exit 0
