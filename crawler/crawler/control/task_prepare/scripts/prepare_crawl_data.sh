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

# 输入目录, param
DEFINE_string 'pv_log_root' '/app/user_data/pv_log' 'pv log HDFS 输入根路径' 'p'
DEFINE_string 'search_log_root' '/app/user_data/search_log' 'search log HDFS 输入根路径' 's'
DEFINE_string 'fetcher_fetched_log_root' '/user/crawler/general_crawler_output/' '任务 driven 的 fetched log HDFS 输入根路径' 'f'
DEFINE_string 'web_db_root' '/app/webdb/brief/' '任务 driven 的 web db HDFS 输入根路径' 'w'
DEFINE_integer 'traceback_days' 20 '考虑爬虫 |traceback_days| 天的抓取结果'
# 输出目录
DEFINE_string 'output' "/user/${USER}/crawl_task" 'HDFS 输出路径' 'o'
# pv log date
DEFINE_string 'date' '000000' 'pv 日志的日期 eg. 20120501' 'd'

DEFINE_integer 'reducer_num' 200 'reducer num'
DEFINE_string 'split_type' 'auto' '[auto|manual], auto 表示自动分割优先级, manual 表示手工指定优先级'
DEFINE_integer 'set_level_to' 1 'in |manual| mode, set level'
DEFINE_integer 'priority_levels' 5 'priority levels'

# 临时目录
DEFINE_string 'hdfs_tmp_dir' "/tmp/${USER}_${HOSTNAME}/${timestamp}/" \
              'HDFS 临时目录，以\$HOSTNAME \$USER 和 \$timestamp 命名' 'T'
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ./${0} --help
  exit 1
fi
set -x
if [[ ${FLAGS_date} != '*' ]];then
  if ! is_valid_date "${FLAGS_date}"; then
    EchoAndDie "Invalid date format"
  fi
fi
[[ -z ${FLAGS_output} ]] && EchoAndDie "Invalid output dir"
(( ${FLAGS_reducer_num} < 1 || ${FLAGS_reducer_num} < ${FLAGS_priority_levels} )) && EchoAndDie "Invalid"
[[ "${FLAGS_split_type}" != "auto" && "${FLAGS_split_type}" != "manual" ]] && EchoAndDie "Invalid split_type"
[[ ${FLAGS_priority_levels} -ne 5 ]] && EchoAndDie "must be 5 levels priority"
(( ${FLAGS_priority_levels} < ${FLAGS_set_level_to} || ${FLAGS_set_level_to} < 1 )) && EchoAndDie "Invalid"
if [[ ${FLAGS_split_type} == auto ]];then
  (( ${FLAGS_priority_levels} < 3 )) && EchoAndDie "Invalid param"
fi
[[ -z ${FLAGS_fetcher_fetched_log_root} ]] && EchoAndDie "Invalid"

# # 从爬虫的输出目录中把一天的数据合并起来
# bash -x merge_fetched_status.sh ${FLAGS_fetcher_fetched_log_root} ${FLAGS_apps_fetched_log_root}
# if [[ $? -ne 0 ]];then
#   EchoAndDie "merge fetched status failed"
# fi

# 从 PvLog, SearchLog, FetchedLog 中生成本次任务的待抓取 url
uv_input=""
${HADOOP_HOME}/bin/hadoop dfs -test -e "${FLAGS_pv_log_root}/${FLAGS_date}_00_00.st" && \
   uv_input="${uv_input}  ${FLAGS_pv_log_root}/${FLAGS_date}_00_00.st"
${HADOOP_HOME}/bin/hadoop dfs -test -e "${FLAGS_search_log_root}/${FLAGS_date}_00_00.st" && \
   uv_input="${uv_input} ${FLAGS_search_log_root}/${FLAGS_date}_00_00.st"
[[ -z ${uv_input} ]] && EchoAndDie "input files not found"

newest_result=`${HADOOP_HOME}/bin/hadoop dfs -ls ${FLAGS_fetcher_fetched_log_root}/ | \
               sed 1d | tail -1 | awk '{print $NF}'`
use_faildata_date=`basename ${newest_result}`
is_valid_date ${use_faildata_date} || EchoAndDie "Invalid date"
for i in `seq 0 ${FLAGS_traceback_days}`;do
  concern_day=`n_days_before ${use_faildata_date} ${i} "%Y%m%d"`
  ${HADOOP_HOME}/bin/hadoop dfs -test -e ${FLAGS_fetcher_fetched_log_root}/${concern_day}
  if [[ $? -ne 0 ]];then
    continue
  fi
  s=`HadoopDus ${FLAGS_fetcher_fetched_log_root}/${concern_day}`
  if ((s > 0));then
    uv_input="${uv_input} ${FLAGS_fetcher_fetched_log_root}/${concern_day}/[0-2]*/status/*.st"
  fi
done

uv_output=${FLAGS_hdfs_tmp_dir}/uv_output.st
bash -x ./uv_data.sh "${uv_input}" ${uv_output} \
        "${FLAGS_date}" ${FLAGS_reducer_num} \
        `norm_path ${FLAGS_pv_log_root}` `norm_path ${FLAGS_search_log_root}` \
        `norm_path ${FLAGS_fetcher_fetched_log_root}` ${use_faildata_date}
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to compute UV data"
fi

# 和网页库去重, 并按照 uv 排序
sort_output=${FLAGS_hdfs_tmp_dir}/sorted.st
web_db=`${HADOOP_HOME}/bin/hadoop dfs -ls ${FLAGS_web_db_root} | grep "_00_00.sf"  | tail -1 | awk '{print $NF}'`
bash -x ./sort.sh ${uv_output} ${sort_output} ${FLAGS_reducer_num} ${web_db}
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to sort data"
fi

bash -x ./split_by_pri.sh ${sort_output} ${FLAGS_output} "${FLAGS_date}" \
                          ${FLAGS_reducer_num} ${FLAGS_priority_levels} ${FLAGS_split_type} \
                          ${FLAGS_set_level_to}
if [ $? -ne 0 ]; then
  EchoAndDie "[CRAWLER_ERROR] Failed to cut data"
fi

${HADOOP_HOME}/bin/hadoop dfs -rmr ${FLAGS_hdfs_tmp_dir}

exit 0
