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
DEFINE_string 'onebox_bot' '/app/feature/tasks/page_analyze/data/robots' 'robots 数据, sfile 格式'
DEFINE_string 'urls' '' '待查询 robots 协议的 url, 需要是 streaming-text 格式'
DEFINE_integer 'url_field' 0 'url 所在字段'
DEFINE_string 'output' "" '输出成 streaming-text 格式' 'o'

DEFINE_string 'spider_type' 'GeneralSpider' '[360Spider|GeneralSpider|RushSpider] 广义爬虫, 360 爬虫, 暴走爬虫
                360 爬虫严格按照 robots 协议抓取, UA 为 360Spider
                广义爬虫, 如果不不允许 360Spider 抓取, 但是允许 BaiduSpider, Googlebot 抓取, 这些难不倒广义爬虫
                暴走爬虫不知道什么叫做 robots 协议'

DEFINE_integer 'robots_level_field' 1 '输出 robots level 的字段'
DEFINE_integer 'output_can_fetch' 1 '1, 输出抓取, 非 1, 输出不抓取'

# 临时目录
DEFINE_string 'hdfs_tmp_dir' "/tmp/${USER}_${HOSTNAME}/${timestamp}" \
              'HDFS 临时目录，以\$HOSTNAME \$USER 和 \$timestamp 命名' 'T'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x

onebox_bot_path=`${HADOOP_HOME}/bin/hadoop dfs -ls ${FLAGS_onebox_bot} | sed '1d' | tail -1 | awk '{print $NF}'`
[[ -z ${onebox_bot_path} ]] && EchoAndDie "No robots.txt found"

input_size=`HadoopDus "${onebox_bot_path} ${FLAGS_urls}"`
((r_task_num=input_size*4/1024/1024/1024))
if ((r_task_num < 1));then
  r_task_num=1
fi
# data prepare
# compress output data to meet the demand of the successive map-only process
mid_output=${FLAGS_hdfs_tmp_dir}/urls_rob.st
${HADOOP_HOME}/bin/hadoop dfs -rmr ${mid_output}
build_tools/mapreduce/submit.py \
  --mr_files ./data/big_family_host.txt \
  --mr_map_cmd data_prepare \
               --bots_prefix ${FLAGS_onebox_bot} \
               --url_field ${FLAGS_url_field} \
               --logtostderr \
  --mr_input ${onebox_bot_path} ${FLAGS_urls} \
  --mr_reduce_cmd data_prepare \
                  --logtostderr \
  --mr_output ${mid_output} \
  --mr_output_format streaming_text \
  --mr_reduce_tasks ${r_task_num} \
  --mr_compress_st_output \
  --mr_job_name "onebox_bot_prepare"
if [ $? -ne 0 ];then
  echo "one box bot prepare fail" >&2
  exit 1
fi

# robots parse
parse_output=${FLAGS_hdfs_tmp_dir}/parsed.st
$HADOOP_HOME/bin/hadoop fs -rmr ${parse_output}
$HADOOP_HOME/bin/hadoop jar ${HADOOP_STREAMING_JAR} \
  -file ./*.sh \
  -file ./*.py \
  -input ${mid_output} \
  -output ${parse_output} \
  -mapper "python robots_parser_mapper.py ${FLAGS_url_field} ${FLAGS_spider_type} ${FLAGS_robots_level_field} ${FLAGS_output_can_fetch}" \
  -jobconf mapred.reduce.tasks=0 \
  -jobconf mapred.min.split.size=50737418240 \
  -jobconf mapred.job.name="onebox_bot_parse"
[[ $? -ne 0 ]] && exit 1

${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_output}
${HADOOP_HOME}/bin/hadoop dfs -test -e `dirname ${FLAGS_output}` || \
  ${HADOOP_HOME}/bin/hadoop dfs -mkdir `dirname ${FLAGS_output}`
${HADOOP_HOME}/bin/hadoop dfs -mv ${parse_output} ${FLAGS_output}
[[ $? -ne 0 ]] && exit 1

${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_hdfs_tmp_dir}

exit 0
