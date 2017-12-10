#!/bin/bash
if [ $# -ne 2 ];then
  echo "Usage: ./merge_fetched_status.sh <fetched_root> <output_root>"
  exit 1
fi
source ./shell/common.sh

fetched_root=$1
output_root=$2

rm -f use_faildata_date
rm -r filelist.*.p*

# 检查 crawler 处理的最新数据的日期
donelist=`${HADOOP_HOME}/bin/hadoop dfs -ls ${fetched_root}/fetcher_*/output/p*/donelist | \
   sort -k6,6 -r | \
   head -1 | \
   awk '{print $NF}'`
if [[ -z ${donelist} ]];then
  echo "Warning: no donelist of fetcher" >&2
  exit 0
fi

# sounds like: /user/crawler/general_crawler/fetcher_005/output/p4/20120601140923_000_00025.sf
local_tmp1=`${HADOOP_HOME}/bin/hadoop dfs -cat ${donelist} | tail -1 | awk '{print $1}'`
# sounds like: 20120601140923_000_00025.sf
local_tmp2=`basename ${local_tmp1}`
# sounds like: 20120601
fetched_newest_date=${local_tmp2:0:8}
if ! is_valid_date ${fetched_newest_date};then
  EchoAndDie "Invalid date"
fi

previous_day=`n_days_before ${fetched_newest_date} 1 "%Y%m%d"`
echo ${previous_day} > use_faildata_date
timestamp=$(date +%s_%N)_$$
tmp_output=/tmp/${USER}_${HOSTNAME}/${timestamp}/
${HADOOP_HOME}/bin/hadoop dfs -mkdir ${tmp_output}
for i in `seq 1 5`;do
  ${HADOOP_HOME}/bin/hadoop dfs -ls ${fetched_root}/fetcher_*/output/p${i}/${previous_day}*.status.st | \
    sed '1d' | \
    awk '{print $NF}' > filelist.${previous_day}.p${i}
  ${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_output}/filelist.${previous_day}.p${i} 
  ${HADOOP_HOME}/bin/hadoop dfs -put filelist.${previous_day}.p${i} ${tmp_output}
  [[ $? -ne 0 ]] && EchoAndDie "failed to upload filelist.${previous_day}.p${i}"
done

# 合并到一个目录中
for i in `seq 1 5`;do
  if [ -s filelist.${previous_day}.p${i} ];then
    ./build_tools/mapreduce/combine_st.sh ${tmp_output}/filelist.${previous_day}.p${i} ${tmp_output}/p${i}.st 20
    if [[ $? -ne 0 ]];then
      EchoAndDie "combine failed"
    fi
  fi
done

# mv 到指定目录
${HADOOP_HOME}/bin/hadoop dfs -rmr ${output_root}/p*/${previous_day}_00_00.st
for i in `seq 1 5`;do
  if ${HADOOP_HOME}/bin/hadoop dfs -test -e ${tmp_output}/p${i}.st;then
    ${HADOOP_HOME}/bin/hadoop dfs -test -e ${output_root}/p${i}/
    [[ $? -ne 0 ]] && ${HADOOP_HOME}/bin/hadoop dfs -mkdir ${output_root}/p${i}
    ${HADOOP_HOME}/bin/hadoop dfs -mv ${tmp_output}/p${i}.st ${output_root}/p${i}/${previous_day}_00_00.st
    [[ $? -ne 0 ]] && EchoAndDie "fail to mv ${tmp_output}/p${i}.st"
  fi
done

${HADOOP_HOME}/bin/hadoop dfs -rmr ${tmp_output}

exit 0
