#!/bin/bash
source ./shell/common.sh
source ./shell/shflags.sh
set -u
set +x

print_help=false
if [[ $# -eq 0 ]];then
  print_help=true
fi

# 输入目录, param
DEFINE_string 'input' '/user/crawler/general_crawler/' 'HDFS 输入根路径' 'f'
# DEFINE_string 'input_root' '/user/crawler/general_crawler' 'HDFS 输入根路径' 'f'
# 输出目录
DEFINE_string 'output_root' '/user/crawler/fetched_status/stat/' 'HDFS 输出根路径' 'o'
DEFINE_string 'date' '000000' '统计的数据的日期 eg. 20120501' 'd'
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ./${0} --help
  exit 1
fi
set -x
# [[ -z ${FLAGS_input_root} ]] && EchoAndDie "Invalid"
[[ -z ${FLAGS_output_root} ]] && EchoAndDie "Invalid"

# host \t total_fetched \t success \t fail \t fail_ratio
# input=${FLAGS_input_root}/fetcher_*/output/p*/${FLAGS_date}*.status.st
output=${FLAGS_output_root}/host_stat/${FLAGS_date}_00_00.st
${HADOOP_HOME}/bin/hadoop dfs -rmr ${output}
./build_tools/mapreduce/submit.py \
    --mr_map_cmd ./bin/host_stat \
      --logtostderr=1 \
    --mr_reduce_cmd ./bin/host_stat \
      --logtostderr=1 \
    --mr_input ${FLAGS_input} \
    --mr_output ${output} \
    --mr_reduce_tasks 200 \
    --mr_cache_archives /wly/web.tar.gz#web \
    --mr_job_name "host_stat_${FLAGS_date}"
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to norm UV data"
fi

# sort host_stat by fail_ratio
util/mr/tera_sort/tera_sort.sh --input "${output}" \
                               --output "${output}" \
                               --reverse --field_id 1 --type "double" \
                               --average_input_record_length 40 \
                               --balance_record_on "item_num"
if [ $? -ne 0 ];then
  echo "fail_ratio sorting failed" >&2
  exit 1
fi

exit 0
