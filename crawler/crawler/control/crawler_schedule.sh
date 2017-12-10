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
DEFINE_string 'log_date' '' 'log(pv/search) date' 'd'

DEFINE_boolean 'task_prepare' true 'prepare uv data from logs(pv log, search log, fetched log)'
DEFINE_boolean 'schedule' true 'schedule for fetcher'
DEFINE_boolean 'main_page' true 'schedule for main page'
DEFINE_boolean 'title_schedule' true 'schedule for title data'
DEFINE_integer 'crawl_log_trace_back' 20 'crawl output log taken into consideration' 'b'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ./${0} --help
  exit 1
fi
set -x
is_valid_date ${FLAGS_log_date}
[[ $? -ne 0 ]] && EchoAndDie "Invalid pv log date"


# uv 数据生成
if [[ ${FLAGS_task_prepare} == 0 ]];then
  cd task_prepare;
  # stat
  rm -f uv_selected_stat_file.${FLAGS_log_date}
  bash -x ./prepare_crawl_data.sh -d ${FLAGS_log_date} --traceback_days ${FLAGS_crawl_log_trace_back}
  if [[ $? -ne 0 ]];then
    EchoAndDie "prepare crawl data failed"
  fi
  uv_selected_num=`grep -a 'Reduce output records' uv_selected_stat_file.${FLAGS_log_date} | awk -F" " '{print $6}'`
  cd -
fi

# 调度
# 根据 uv 数据, 为 fetcher 生成抓取任务
idx=0
if [[ ${FLAGS_schedule} == 0 ]];then
  cd schedule;
  for priority in `seq 5 -1 1`;do
    dir_size=`HadoopDus /user/${USER}/crawl_task/${FLAGS_log_date}_p${priority}.st`
    if [[ ${dir_size} -gt 0 ]];then
      timestamp=`date "+%Y%m%d%H%M%S"`
      rm -f dns_input_stat_file.*.${timestamp}
      rm -f dns_output_stat_file.*.${timestamp}
      dns_input_array[$idx]=dns_input_stat_file.${priority}.${timestamp}
      dns_output_array[$idx]=dns_output_stat_file.${priority}.${timestamp}
      ((idx++))
      bash -x ./run_schedule.sh ./schedule.conf \
          ${priority} 100000 \
          /user/${USER}/crawl_task/${FLAGS_log_date}_p${priority}.st ""
      if [[ $? -ne 0 ]];then
        EchoAndDie "prepare crawl data failed"
      fi
    fi
  done
  cd -
fi

exit 0

# main page
# 概率去抓取
ra=$RANDOM
if [[ ${FLAGS_main_page} == 0 && ${ra} -lt 15000 ]];then
  input=`${HADOOP_HOME}/bin/hadoop dfs -ls /app/feature/tasks/page_analyze/data/main_page/daily/ | sed '1d' | tail -1 | awk '{print $NF}'`
  if [ -z ${input} ];then
    exit 0
  fi
  output=/tmp/crawler_mp_${HOSTNAME}/`date +%Y%m%d`_$$_mp.st
  $HADOOP_HOME/bin/hadoop fs -rmr ${output}
  $HADOOP_HOME/bin/hadoop jar ${HADOOP_STREAMING_JAR} \
    -file ./*.awk \
    -input ${input} \
    -output ${output} \
    -mapper 'awk -f site_to_crawler_input.awk' \
    -jobconf mapred.reduce.tasks=0 \
    -jobconf mapred.job.name="reform_site"
  if [[ $? -ne 0 ]];then
    SendAlertViaMail "site schedule reform site fail" huangboxiang@oneboxtech.com
    exit 1
  fi
  cd schedule;
  bash -x ./run_schedule.sh ./schedule.conf 4 100000 ${output} ""
  if [[ $? -ne 0 ]];then
    EchoAndDie "site schedule failed"
  fi
  cd -;
fi

# title
# 去除, 超低概率抓取
ra=$RANDOM
if [[ ${FLAGS_title_schedule} == 0 && ${ra} -lt 10 ]];then
  input=`${HADOOP_HOME}/bin/hadoop dfs -ls /user/huangboxiang/sel_on_title/daily/ | sed '1d' | tail -1 | awk '{print $NF}'`
  if [ -z ${input} ];then
    exit 0
  fi
  output=/tmp/crawler_title_${HOSTNAME}/`date +%Y%m%d`_$$_title.st
  $HADOOP_HOME/bin/hadoop fs -rmr ${output}
  $HADOOP_HOME/bin/hadoop jar ${HADOOP_STREAMING_JAR} \
    -file ./*.awk \
    -input ${input} \
    -output ${output} \
    -mapper 'awk -f uv_to_crawler_input.awk' \
    -jobconf mapred.reduce.tasks=0 \
    -jobconf mapred.job.name="reform_title"
  if [[ $? -ne 0 ]];then
    SendAlertViaMail "title schedule reform title fail" huangboxiang@oneboxtech.com
    exit 1
  fi
  cd schedule;
  bash -x ./run_schedule.sh ./schedule.conf 1 100000 ${output} ""
  if [[ $? -ne 0 ]];then
    EchoAndDie "title schedule failed"
  fi
  cd -;
fi

# 发送报表

exit 0
