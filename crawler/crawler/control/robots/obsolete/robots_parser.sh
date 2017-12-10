#!/bin/bash

source ./shell/common.sh
source ./shell/shflags.sh
set -u
set +x

print_help=false
if [[ $# -eq 0 ]];then
  print_help=true
fi

timestamp=$(date +%s_%N)_$$

# 输入，输出目录
DEFINE_string 'crawled_robots' '/user/crawler/special_crawler_individual/fetcher_000/output/p5/*.sf' '抓取下来的robots数据'
DEFINE_string 'webdb' '/user/pengdan/webdb_url/20120823.st' 'webdb'
DEFINE_string 'output' '' '输出成streaming-text file' 'o'

# 临时目录
DEFINE_string 'hdfs_tmp_dir' "/tmp/${USER}_${HOSTNAME}/${timestamp}/scores.st" \
              'HDFS 临时目录，以\$HOSTNAME \$USER 和 \$timestamp 命名' 'T'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x

r_task_num=5009
${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_output}
build_tools/mapreduce/submit.py \
  --mr_map_cmd robots_parser \
               --logtostderr \
  --mr_input ${FLAGS_crawled_robots} ${FLAGS_webdb} \
  --mr_reduce_cmd robots_parser \
                  --logtostderr \
  --mr_output ${FLAGS_output} \
  --mr_min_split_size_in_mb 10240 \
  --mr_output_format streaming_text \
  --mr_reduce_tasks ${r_task_num} \
  --mr_job_name "robots"
if [ $? -ne 0 ];then
  echo "scores calc failed" >&2
  exit 1
fi

exit 0
