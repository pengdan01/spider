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
DEFINE_string 'date' 000000 'set date'
# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"
if [[ ${print_help} == true ]];then
  ${0} --help
  exit 1
fi
set -x

while true;do
  is_valid_date ${FLAGS_date}
  if [[ $? -ne 0 ]];then
    use_date=`date -d '1 days ago' +%Y%m%d`
  else
    use_date=${FLAGS_date}
  fi

  site_uv=/app/feature/tasks/page_analyze/data/site_uv/daily/${use_date}_00_00.st
  retry=0
  while true;do
    ${HADOOP_HOME}/bin/hadoop dfs -test -e ${site_uv}
    ret1=$?
    if [ ${ret1} -eq 0 ];then
      break
    else
      sleep 60
      ((retry++))
      if ((retry > 60));then
        echo "Nothing new" >&2
        break
      fi
    fi
  done
  if ((retry > 60));then
    continue;
  fi

  ./host_ip_query_regular.sh --to_date ${use_date}
  FLAGS_date=000000
  sleep 86400
done
