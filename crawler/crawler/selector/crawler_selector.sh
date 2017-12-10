#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error: Usage: $0 <conf-file>" && exit 1
fi
conf_file=$1
if [ ! ${conf_file} -o ! -e ${conf_file} ]; then
  echo "Error: parameter conf_file[${conf_file}] is NULL or file conf_file not exist." && exit 1
fi
## 加载 shell 基础库函数
shell_file=shell/common.sh
[ ! -e ${shell_file} ] && echo "Error, shell_file[${shell_file}] not find" && exit 1

source ${shell_file}
source ${conf_file}

[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

## 检查 文件锁 是否存在 并加锁
${HADOOP_HOME}/bin/hadoop fs -test -e ${lock_hdfs_file}
## 任务还在运行中 或者 上一次任务没有正常结束
if [ $? -eq 0 ]; then
  err_msg="Selector is running or aborted in last time, as find lock file: ${lock_hdfs_file}" 
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi
## 上一次任务正常结束, 本次任务可以正常开始, 先加锁
${HADOOP_HOME}/bin/hadoop fs -touchz ${lock_hdfs_file}
if [ $? -ne 0 ]; then
  err_msg="Fail: hadoop fs -touchz ${lock_hdfs_file}" 
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi

timestamp=`date "+%F-%H%M%S"`
source crawler/selector/selector_round1.sh

## Add to selector donelist
${HADOOP_HOME}/bin/hadoop fs -test -e ${selector_donelist}
if [ $? -eq 0 ]; then
  rm -f ${status_dir}/selector_donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -get ${selector_donelist} ${status_dir}/selector_donelist.tmp
fi

record="${Round1_output}\t${selector_input}\t${timestamp}"
echo -e ${record} >> ${status_dir}/selector_donelist.tmp

${HADOOP_HOME}/bin/hadoop fs -rm ${selector_donelist}

${HADOOP_HOME}/bin/hadoop fs -put ${status_dir}/selector_donelist.tmp ${selector_donelist}
if [ $? -ne 0 ]; then
  err_msg="Fail: hadoop fs -put ${status_dir}/selector_donelist.tmp ${selector_donelist}"
  echo "${err_msg}"
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1 
fi  

## 成功退出, 释放文件锁
${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}

done_msg="Done, append record to selector donelist: ${record}"
echo "${done_msg}"
SendAlertViaMail "${done_msg}" "${alert_mail_receiver}"

exit 0
