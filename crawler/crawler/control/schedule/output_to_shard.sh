set -u

if [ $# -ne 4 ]; then
  echo "Usage: $0 配置文件 时间戳 优先级 输出路径"
  exit 1
fi

conf_file=$1
timestamp=$2
priority=$3
shard_output_dir=$4

source ${conf_file}

# 获得 shard_number_info
shard_number=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_number_info} | grep "page_shard_num" | awk -F"=" '{print $2}'`
if [ $? -ne 0 ]; then
  echo "cat shard_number_info file ${shard_number_info} failed"
  exit 1
fi

if [ ! "${timestamp}" ];then
  echo "timestamp should not be null"
  exit 1
fi

((tmp = fetcher_node_num-1))

# 检查是否所有的 fetcher 都跑完
while [ 1 ]; do
  sleep 60
  echo "should sleep!!!"
  all_finished=1
  for i in `seq 0 ${tmp}`;
  do
    # 构造本地任务的输入和输出路径
    fetcher_i_str=`printf %.3d ${i}`
    input_path=${fetcher_input_path}/fetcher_${fetcher_i_str}/input/p${priority}
    output_path=${fetcher_input_path}/fetcher_${fetcher_i_str}/output/p${priority}
    input_file_list=`$HADOOP_HOME/bin/hadoop fs -cat ${input_path}/donelist | grep "${timestamp}" | awk -F"_" '{print $NF}' | sort -n`
    output_file_list=`$HADOOP_HOME/bin/hadoop fs -cat ${output_path}/donelist | grep "${timestamp}" | awk -F"\t" '{print $3}' | awk -F"_" '{print $NF}' | sort -n`
    if [ ${input_file_list} != ${output_file_list} ]; then
      all_finished=0
      break
    fi
  done
  if [ ${all_finished} -eq 1 ];then
    echo "all fetchers's task are finished"
    break
  fi
done

echo "start moving to shard"

to_shard_input_data_path="${fetcher_input_path}/fetcher_*/output/p${priority}/${timestamp}*.output/page.sf/*"
to_shard_output_data_path="${crawler_schedule_output_path}/${timestamp}.output.hf"

${HADOOP_HOME}/bin/hadoop fs -rmr ${to_shard_output_data_path}
build_tools/mapreduce/submit.py \
    --mr_map_cmd ./output_to_shard --shard_num=${shard_number} \
    --mr_reduce_cmd ./output_to_shard --shard_num=${shard_number} \
    --mr_input ${to_shard_input_data_path}  \
    --mr_output ${to_shard_output_data_path} \
    --mr_reduce_tasks=50 \
    --mr_input_format sfile \
    --mr_output_format hfile \
    --mr_multi_output \
    --mr_job_name crawler_schedule_to_shard
if [ $? -ne 0 ]; then
  echo "run hadoop job crawler_schedule_to_shard failed"
  exit 1
fi

echo "task crawler_schedule_to_shard finished, start to move data to output directory..."

((tmp=shard_number-1))
for i in `seq 0 $tmp`;
do
  #查看 shard 是否存在 不存在则创建目录
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir} failed"
      exit 1
    fi
  fi
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}/page
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir}/page doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}/page
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir}/page failed"
      exit 1
    fi
  fi
  shard_id=`printf %.3d ${i}`
  $HADOOP_HOME/bin/hadoop fs -test -d ${shard_output_dir}/page/shard_${shard_id}
  if [ $? -ne 0 ]; then
    echo "${shard_output_dir} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${shard_output_dir}/page/shard_${shard_id}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${shard_output_dir}/page/shard_${shard_id} failed"
      exit 1
    fi
  fi
  target_output_path=${shard_output_dir}/page/shard_${shard_id}/${timestamp}.hf
  $HADOOP_HOME/bin/hadoop fs -test -d ${target_output_path}
  if [ $? -ne 0 ]; then
    echo "${target_output_path} doesn't exist, will create it!"
    $HADOOP_HOME/bin/hadoop fs -mkdir ${target_output_path}
    if [ $? -ne 0 ]; then
      echo "hadoop fs -mkdir ${target_output_path} failed"
      exit 1
    fi
  fi
  $HADOOP_HOME/bin/hadoop fs -mv ${to_shard_output_data_path}/shard-${i}* ${target_output_path}
  if [ $? -ne 0 ]; then
    echo "hadoop fs -mv ${to_shard_output_data_path}/shard-${i}-* to ${target_output_path} failed"
    exit 1
  fi
done

echo "Done moving to shard"
