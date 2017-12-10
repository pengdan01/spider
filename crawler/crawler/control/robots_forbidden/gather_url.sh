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
# 用户浏览
DEFINE_string 'uv_rank_path' '' 'HDFS 输入路径，必须是 streaming-text 格式'
# 搜索引擎
DEFINE_string 'show_rank_path' '' 'HDFS 输入路径'
# robots 协议
DEFINE_string 'robots_path' '/app/feature/tasks/page_analyze/data/robots' 'HDFS 输入路径'
# redis 机器列表
DEFINE_string 'redis_ips_file' './data/redis_url_servers.txt' 'redis 机器列表'

DEFINE_string 'output' '' 'HDFS 输出路径的根目录' 'o'

# 临时目录
DEFINE_string 'hdfs_tmp_dir' "/tmp/${USER}_${HOSTNAME}/${timestamp}/" \
              'HDFS 临时目录，以\$HOSTNAME \$USER 和 \$timestamp 命名' 'T'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x

# 结合 uv 和 show 数据, 生成 url level score
input_size=`HadoopDus "${FLAGS_uv_rank_path} ${FLAGS_show_rank_path}"`
((r_task_num = input_size/1024/1024/1024/2))
if [ ${r_task_num} \< 1 ];then
  r_task_num=1
fi
url_level_score=${FLAGS_hdfs_tmp_dir}/url_level_score.st
uv_rank_prefix=$(dirname $(echo ${FLAGS_uv_rank_path} | awk '{print $1;}'))
show_rank_prefix=$(dirname $(dirname $(echo ${FLAGS_show_rank_path} | awk '{print $1;}')))
${HADOOP_HOME}/bin/hadoop dfs -rmr ${url_level_score}
build_tools/mapreduce/submit.py \
  --mr_map_cmd gather_url --uv_lowerbound 30 --path_depth 2 \
               --uv_rank_prefix ${uv_rank_prefix} \
               --show_rank_prefix ${show_rank_prefix} \
               --logtostderr \
  --mr_input ${FLAGS_uv_rank_path} ${FLAGS_show_rank_path} \
  --mr_reduce_cmd gather_url \
                  --logtostderr \
  --mr_output ${url_level_score} \
  --mr_output_format streaming_text \
  --mr_reduce_tasks ${r_task_num} \
  --mr_min_split_size_in_mb 2048 \
  --mr_job_name "url_level_score"
if [ $? -ne 0 ];then
  echo "url level score failed" >&2
  exit 1
fi

# 过 robots 协议
robots_forbidden=${FLAGS_hdfs_tmp_dir}/robots_forbidden.st
cd ../robots
./OneboxBot_filter.sh \
   --onebox_bot ${FLAGS_robots_path} \
   --urls ${url_level_score} \
   --url_field 0 \
   --spider_type GeneralSpider \
   --robots_level_field -10 \
   --output_can_fetch 0 \
   --output ${robots_forbidden}
if [ $? -ne 0 ];then
  echo "robots failed" >&2
  exit 1
fi
cd -

# 查询 redis, 拼接 graph 信息
local_url_level_score=./url_level_score.${timestamp}.$$
local_resource_graph_output=./url_resource_graph.${timestamp}.$$
${HADOOP_HOME}/bin/hadoop dfs -cat ${robots_forbidden}/part-* > ${local_url_level_score} || \
    EchoAndDie "robots forbidden fail to download to local"
cat ${local_url_level_score} | ./resource_graph \
   --output ${local_resource_graph_output} \
   --ds_redis_ips_file ${FLAGS_redis_ips_file} \
   --logbuflevel -1
if [[ $? -ne 0 ]];then
  EchoAndDie "robots forbidden fail on local resource graphing"
fi

# upload to hdfs, and change to sfile format
hdfs_resource_graph=${FLAGS_hdfs_tmp_dir}/resource_graph
${HADOOP_HOME}/bin/hadoop dfs -rm ${hdfs_resource_graph}
bash ./build_tools/mapreduce/sfile_writer.sh -output ${hdfs_resource_graph} < ${local_resource_graph_output}
[[ $? -ne 0 ]] && EchoAndDie "robots forbidden fail to sfile write to hdfs"

${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_output}
${HADOOP_HOME}/bin/hadoop dfs -mkdir ${FLAGS_output} && \
${HADOOP_HOME}/bin/hadoop dfs -mv ${hdfs_resource_graph} ${FLAGS_output}
if [[ $? -ne 0 ]];then
  EchoAndDie "robots forbidden, fail to mv to output"
fi

rm *.${timestamp}.$$
${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_hdfs_tmp_dir}

exit 0
