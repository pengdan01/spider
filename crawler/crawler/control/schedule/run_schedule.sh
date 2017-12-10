#! /bin/bash
set -u

if [ $# -ne 5 ]; then
  echo "Error: Usage: $0 配置文件地址 优先级 单任务URL个数 输入URL地址(多个地址用逗号分隔) 输出地址" && exit 1
fi
conf_file_path=$1
priority=$2
url_num_per_task=$3
input_data_path=$4
output_data_path=$5

source ${conf_file_path}
if [ ${timestamp} = "0" ];then
  timestamp=`date "+%Y%m%d%H%M%S"`
fi
echo "timestamp: ${timestamp}"

# 先使用 robots 协议过滤
# 加入 robots level field
bot_crawlable_path=/tmp/${USER}/bot_crawlable_${timestamp}_$$.st/
cd ../robots/
./OneboxBot_filter.sh \
  --onebox_bot ${robots_path} \
  --urls ${input_data_path} \
  --url_field 0 \
  --spider_type ${spider_type} \
  --robots_level_field 10 \
  --output ${bot_crawlable_path}
if [[ $? -ne 0 ]];then
  echo "robots parser fail" >&2
  exit 1
fi
cd -

# 本脚本对输入路径有个强制要求：
# 1. 输入路径不能与配置文件中的 crawler_schedule_output_path 以及 host_load_control_file 处于同一目录
# 这样的目的是 在做 host 到 dns ip 的拼接时能正确拼接 host 的 ip

input_data_path_dir=`dirname ${bot_crawlable_path}`
crawler_schedule_output_path_dir=`dirname ${crawler_schedule_output_path}`
host_load_control_file_dir=`dirname ${host_load_control_file}`
if [ ${input_data_path_dir} = ${crawler_schedule_output_path_dir} -o ${input_data_path_dir} = ${host_load_control_file_dir} ]; then
  echo "输入文件路径必须与调度模块输出目录以及 host_load_control 所以路径不同"
  exit 1
fi
# 判断优先级是否大于0 小于 9, 以及是否是数据
check_num_tmp_var=`echo $priority | grep -E "^[0-9]$"`
if [ "${check_num_tmp_var}" == "" ]; then
  echo "priority should be numbers and in range [0, 9]"
  exit 1
fi
if [ ${priority} -lt 0 -o ${priority} -gt 9 ]; then
  echo "priority should be [0, 9]"
  exit 1
fi


# 检查 fetcher_node_num 是否合法
check_num_tmp_var=`echo $fetcher_node_num | grep -E "^[0-9]+$"`
if [ "${check_num_tmp_var}" == "" ]; then
  echo "fetcher_node_num should be numbers"
  exit 1
fi
if [ $fetcher_node_num -lt 0 ]; then
  echo "fetcher_node_num should larger than 0"
  exit 1
fi

# 检查配置所指定的输出目录是否存在，如果不存在则创建
$HADOOP_HOME/bin/hadoop fs -test -d ${crawler_schedule_output_path}
if [ $? -ne 0 ]; then
  echo "${crawler_schedule_output_path} doesn't exist, will create it!"
  $HADOOP_HOME/bin/hadoop fs -mkdir ${crawler_schedule_output_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mkdir ${crawler_schedule_output_path} failed"
    exit 1
  fi
fi

# 构建输出目录
base_output_data_path=${crawler_schedule_output_path}/${timestamp}

# 统计待抓取 url 的 host 列表
url_host_output_data_path=${base_output_data_path}/url_host.st
echo "Start run url host stat"
${HADOOP_HOME}/bin/hadoop fs -rmr ${url_host_output_data_path}
build_tools/mapreduce/submit.py \
    --mr_map_cmd ./dns_query_on_hadoop \
    --mr_reduce_cmd ./dns_query_on_hadoop \
    --mr_input ${bot_crawlable_path} \
    --mr_output ${url_host_output_data_path} \
    --mr_input_format streaming_text \
    --mr_output_format streaming_text \
    --mr_reduce_tasks=5 \
    --mr_output_counters=./dns_input_stat_file.${priority}.${timestamp} \
    --mr_job_name crawler_host_stat
if [ $? -ne 0 ]; then
  echo "crawler host stat failed"
  exit 1
fi
../HostIp/host_ip_query_tipical.sh \
    --host_ip_db_root ${host_ip_info} \
    --hosts ${url_host_output_data_path} \
    --dns_servers ${dns_servers} \
    --window_size 300 \
    --max_retry 1 \
    --local_output ./url_host_dns_${timestamp}
if [ $? -ne 0 ]; then
  echo "crawler dns queries failed"
  exit 1
fi

# 调用 multi dns 模块
cd ./multidns/
bash all_in_one.sh &> log
if [ $? -ne 0 ]; then
  echo "multi dns failed"
  exit 1
fi
cd ..
python ./dns_output_merge.py ./multidns/validate/ multi_dns.${timestamp}
if [ $? -ne 0 ]; then
  echo "multi dns merge failed"
  exit 1
fi
python ./merge_dns.py multi_dns.${timestamp} ./url_host_dns_${timestamp} ./dns_black_list ./url_host_dns_merge_${timestamp}
if [ $? -ne 0 ]; then
  echo "merge dns failed"
  exit 1
fi

# 使用刚刚查询得到的DNS，拼接待抓取的 url， host_load文件
# 首先上传 dns ip 结果
url_dns_input_data_path=${base_output_data_path}/url_dns.st
url_ip_output_data_path=${base_output_data_path}/url_ip.st
host_dns_ip=${url_dns_input_data_path}
crawler_task_input_path=${bot_crawlable_path}
host_load_control=${host_load_control_file}
if [ ${run_url_to_ip} = "y" ]; then
  echo "Run url_to_ip"
  ${HADOOP_HOME}/bin/hadoop fs -test -d ${url_dns_input_data_path}
  if [ $? -eq 0 ]; then
    echo "${url_dns_input_data_path} already exists, move to ${url_dns_input_data_path}.bak"
    ${HADOOP_HOME}/bin/hadoop fs -mv ${url_dns_input_data_path} ${url_dns_input_data_path}.bak
    if [ $? -ne 0 ]; then
      echo "move ${url_dns_input_data_path} to ${url_dns_input_data_path}.bak failed"
      exit 1
    fi
  fi
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${url_dns_input_data_path}
  if [ $? -ne 0 ]; then
    echo "url dns mkdir ${url_dns_input_data_path} failed"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -put ./url_host_dns_merge_${timestamp} ${url_dns_input_data_path}
  if [ $? -ne 0 ]; then
    echo "url dns upload failed"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${url_ip_output_data_path}
  build_tools/mapreduce/submit.py \
      --mr_map_cmd ./url_to_ip \
      --crawler_task_input_path=${input_data_path_dir} \
      --host_ip_path=${host_dns_ip} \
      --mr_reduce_cmd ./url_to_ip \
      --crawler_task_input_path=${input_data_path_dir} \
      --host_ip_path=${host_dns_ip} \
      --mr_input ${bot_crawlable_path},${host_dns_ip} \
      --mr_output ${url_ip_output_data_path} \
      --mr_input_format streaming_text \
      --mr_output_format streaming_text \
      --mr_reduce_tasks=30 \
      --mr_multi_output \
      --mr_job_name crawler_url_ip
  if [ $? -ne 0 ]; then
    echo "hadoop url_to_ip task failed"
    exit 1
  fi
else
  echo "Jump url_to_ip"
fi

ip_url_split_input=${url_ip_output_data_path}/crawler_task-*
ip_url_split_output=${base_output_data_path}/ip_url_split.st
if [ ${run_ip_url_split} = "y" ]; then
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${ip_url_split_output}
  build_tools/mapreduce/submit.py \
      --mr_map_cmd ./ip_url_split \
      --proxy_white_list_path=./proxy_white_list \
      --mr_reduce_cmd ./ip_url_split \
      --proxy_white_list_path=./proxy_white_list \
      --mr_input ${ip_url_split_input} \
      --mr_output ${ip_url_split_output} \
      --mr_input_format streaming_text \
      --mr_output_format streaming_text \
      --mr_reduce_tasks=10 \
      --mr_multi_output \
      --mr_files=./proxy_white_list \
      --mr_cache_archives /wly/web.tar.gz#web \
      --mr_job_name crawler_ip_url_split
  if [ $? -ne 0 ]; then
    echo "hadoop ip_url_split task failed"
    exit 1
  fi
fi
# 获取生成的 ip_load 文件
# 先生成一个本地的临时目录
ip_load_tmp_dir=./${timestamp}.ip_load_tmp
ip_stat_output_data_path=${base_output_data_path}/ip_stat.st
ip_stat_input_data_path=${ip_url_split_output}/*
if [ ${run_ip_stat} = "y" ]; then
  echo "run ip_stat"
  # 统计每个 ip 总共要花多长时间抓取完
  touch ip_load.${timestamp}
  ${HADOOP_HOME}/bin/hadoop fs -rmr ${ip_stat_output_data_path}
  build_tools/mapreduce/submit.py \
      --mr_map_cmd ./ip_stat --ip_load_file_path="./ip_load.${timestamp}" \
      --mr_reduce_cmd ./ip_stat --ip_load_file_path="./ip_load.${timestamp}" \
      --mr_input ${ip_stat_input_data_path} \
      --mr_output ${ip_stat_output_data_path} \
      --mr_input_format streaming_text \
      --mr_output_format streaming_text \
      --mr_reduce_tasks=1 \
      --mr_files ./ip_load.${timestamp} \
      --mr_job_name crawler_ip_load
  if [ $? -ne 0 ]; then
    echo "hadoop task ip_stat failed"
    exit 1
  fi
else
  echo "Jump ip_stat"
fi

if [ ${run_ip_to_fetcher_id} = "y" ]; then
  echo "Run ip_to_fetcher_id"
  ${HADOOP_HOME}/bin/hadoop fs -get ${ip_stat_output_data_path}/part-00000 ./ip_stat.${timestamp}
  if [ $? -ne 0 ]; then
    echo "get ip time consume failed"
    exit 1
  fi
  echo "Start to sort ip_stat"
  # 按时间排序 
  sort -g -k2 -r ./ip_stat.${timestamp} > ./ip_stat.${timestamp}.sort
  if [ $? -ne 0 ]; then
    echo "sort ./ip_stat.${timestamp} failed"
    exit 1
  fi
  echo "start to run ip_to_fetcher_id.py"
  # 根据 fetcher 节点数量计算 ip 应该被分发到哪个节点
  python ./ip_to_fetcher_id.py ./ip_stat.${timestamp}.sort ${fetcher_node_num} ./ip_to_id_file.${timestamp}
  if [ $? -ne 0 ]; then
    echo "run ip_to_fethcer_id.py failed"
    exit 1
  fi
  rm ./ip_stat.${timestamp}
else
  echo "Jump ip_to_fetcher_id"
fi

schedule_input_data_path=${ip_url_split_output}/*
schedule_output_data_path=${base_output_data_path}/schedule.st
if [ ${run_schedule_to_fetcher_id} = "y" ]; then
  echo "Run schedule to fetcher_id"
  ${HADOOP_HOME}/bin/hadoop fs -test -d ${schedule_output_data_path}
  if [ $? -eq 0 ]; then
    echo "${schedule_output_data_path} already exists, mv to ${schedule_output_data_path}.bak"
    ${HADOOP_HOME}/bin/hadoop fs -mv ${schedule_output_data_path} ${schedule_output_data_path}.bak
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mv ${schedule_output_data_path} to ${schedule_output_data_path}.bak failed"
      exit 1
    fi
  fi
  schedule_to_fetcher_id_reducer_num=`echo "${fetcher_node_num} * 5" | bc`
  # 开始运行调度程序
  build_tools/mapreduce/submit.py \
      --mr_map_cmd ./schedule_to_fetcher_id \
      --url_num_per_task=${url_num_per_task} \
      --fetcher_node_num=${fetcher_node_num} \
      --ip_to_fetcher_id_file_path=./ip_to_id_file.${timestamp} \
      --reducer_num=${schedule_to_fetcher_id_reducer_num} \
      --mr_reduce_cmd ./schedule_to_fetcher_id \
      --url_num_per_task=${url_num_per_task} \
      --fetcher_node_num=${fetcher_node_num} \
      --ip_to_fetcher_id_file_path=./ip_to_id_file.${timestamp} \
      --reducer_num=${schedule_to_fetcher_id_reducer_num} \
      --mr_input ${schedule_input_data_path}  \
      --mr_output ${schedule_output_data_path} \
      --mr_reduce_tasks=${schedule_to_fetcher_id_reducer_num} \
      --mr_files "./ip_to_id_file.${timestamp}" \
      --mr_input_format streaming_text \
      --mr_output_format streaming_text \
      --mr_multi_output \
      --mr_output_counters=./dns_output_stat_file.${priority}.${timestamp} \
      --mr_job_name crawler_schedule
  if [ $? -ne 0 ]; then
    echo "run hadoop job crawler_schedule failed"
    exit 1
  fi
else
  echo "Jump schedule to fetcher_id"
fi
echo "crawler_schedule finished, moving output to fetcher input dir..."
# 将结果 move 到目标目录并更新 donelist
# 对每个 fetcher
id=1
parallel=10
((tmp = fetcher_node_num - 1))
for fetcher_i in `seq 0 ${tmp}`;
do
  sh move_to_target.sh ${fetcher_input_path} ${fetcher_i} ${priority} ${timestamp} ${schedule_output_data_path} ./ip_load.${timestamp} &
  pid[$id]=$!
  ((id++))
  if [ $id -gt $parallel ]; then
    for i in ${pid[@]}; do
      wait $i
      [[ $? -ne 0 ]] && exit 1
    done
    echo "finished moving fetcher ${fetcher_i}'s data"
    pid=""
    id=1
  fi
done

for i in ${pid[@]}; do
  wait $i
  [[ $? -ne 0 ]] && exit 1
done
rm ./ip_load.${timestamp}
echo "finished moving all data, now wait for fetchers to crawl..."
exit 0

# 监控每一个 fetcher 的 donelist 如果完成所有任务，则开始做merge
bash output_to_shard.sh ${conf_file_path} ${timestamp} ${priority} ${output_data_path}
if [ $? -ne 0 ]; then
  echo "run output_to_shard.sh failed"
  exit 1
fi

echo "Schedule finished!"
