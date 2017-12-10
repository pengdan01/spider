#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 <dispatcher_conf_file>"
  exit 1
fi
conf_file=$1
if [ ! "${conf_file}" -o ! -e "${conf_file}" ]; then
  echo "Error, conf file[${conf_file}] not exist"
  exit 1
fi
## 加载 shell 基础库函数
shell_file=shell/common.sh
[ ! -e ${shell_file} ] && echo "Error, shell_file[${shell_file}] not find" && exit 1
source ${shell_file}
source ${conf_file}

## 检查 文件锁 是否存在 并加锁
${HADOOP_HOME}/bin/hadoop fs -test -e ${lock_hdfs_file}
## 任务还在运行中 或者 上一次任务没有正常结束
if [ $? -eq 0 ]; then
  err_msg="Error, Dispatcher is running, as find lock file: ${lock_hdfs_file}" 
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

## check donelist
${HADOOP_HOME}/bin/hadoop fs -test -e "${selector_donelist}"
if [ $? -ne 0 ] ; then
  err_msg="Error, selector donelist[${selector_donelist}] not found in hadoop."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi

[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

## 检查并解析 crawler deploy file 获取 downloader worker 数 
if [ ! "${crawler_deploy_conf_file}" -o ! -e "${crawler_deploy_conf_file}" ]; then
  err_msg="Error, crawler deploy conf file[${crawler_deploy_conf_file}] is not exist"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
grep -v "^#" ${crawler_deploy_conf_file} | sed /'^ *$'/d > ${status_dir}/refined_crawler_deploy_file
line_num1=`wc -l ${status_dir}/refined_crawler_deploy_file | awk '{print $1}'`
line_num2=`cat ${status_dir}/refined_crawler_deploy_file | awk '{print $2}' | sort -u | wc -l | awk '{print $1}'`
if [ ${line_num1} -ne ${line_num2} -o ${line_num2} -eq 0 ]; then
  err_msg="Error, crawler deploy file[${crawler_deploy_conf_file}] not valid, crawler worker id should be unique \
  and the deploy file should config at least one worker."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
crawler_worker_num=${line_num1}
id_array=(`cat ${status_dir}/refined_crawler_deploy_file | awk '{print $2}'`)

## 整个crawler 集群的日抓取能力
crawler_cluster_capacity=`expr ${crawler_worker_num} \* ${per_downloader_crawle_ability}`
echo "Info, the crawler cluster crawl capacity per day: ${crawler_cluster_capacity}"

rm -f ${status_dir}/selector_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -get ${selector_donelist} ${status_dir}/selector_donelist.tmp
if [ $? -ne 0 ]; then
  err_msg="Fail: hadoop fs -get ${selector_donelist} ${status_dir}/selector_donelist.tmp fail"
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
## selector donelist 包含了需要抓取的 任务 list
## 检查 selector_donelist 是否有更新
selector_donelist_modified=y
selector_input_dir=""
if [ -e ${status_dir}/selector_donelist ]; then
  diff ${status_dir}/selector_donelist ${status_dir}/selector_donelist.tmp </dev/null &>/dev/null
  if [ $? -eq 0 ]; then
    selector_donelist_modified=n
  else
    last_handled_dir=`tail -1 ${status_dir}/selector_donelist | awk '{print $1}'`
    finish_line=`grep -n "${last_handled_dir}" ${status_dir}/selector_donelist.tmp | awk -F':' '{print $1}'`
    [ ! "${finish_line}" ] && finish_line=0
    cnt=0
    while read line
    do
      ((++cnt))
      tmpdir=`echo $line | awk '{print $1}'`
      [ $cnt -le $finish_line -o ! "${tmpdir}" ] && continue
      if [ "${selector_input_dir}" ]; then
        selector_input_dir=${selector_input_dir},${tmpdir}
      else
        selector_input_dir=${tmpdir}
      fi
    done < ${status_dir}/selector_donelist.tmp 
  fi
fi

#rm -f ${status_dir}/updater_donelist.tmp &>/dev/null
#${HADOOP_HOME}/bin/hadoop fs -get ${updater_donelist} ${status_dir}/updater_donelist.tmp
#if [ $? -ne 0 ]; then
#  echo "hadoop fs -get ${updater_donelist} ${status_dir}/updater_donelist.tmp fail"
#  exit 1
#fi
### updater donelist 包含了需要 Update 的 任务 list
### 检查 updater donelist 是否有更新
#updater_donelist_modified=y
#updater_input_dir=""
#if [ -e ${status_dir}/updater_donelist ]; then
#  diff ${status_dir}/updater_donelist ${status_dir}/updater_donelist.tmp </dev/null &>/dev/null
#  if [ $? -eq 0 ]; then
#    echo "Warning, updater_donelist is the same as local cached, so NO URLs need to CHECK and Update"
#    updater_donelist_modified=n
#  fi
#fi
#if [ ${updater_donelist_modified} = "y" ]; then
#  updater_input_dir=`tail -1 ${status_dir}/updater_donelist.tmp | awk -F'\t' '{print $1}'`
#fi
#if [ ${selector_donelist_modified} = "n" -a ${updater_donelist_modified} = "n" ]; then
#  echo "Both selector donelist and updater donelist not modified, So no tasks need to do"
#  exit 0
#fi
if [ ${selector_donelist_modified} = "n" -o ! "${selector_input_dir}" ]; then
  warning_msg="Warning, selector_donelist is the same as local cached, so NO new urls need to fetch"
  echo ${warning_msg}
  SendAlertViaMail "${warning_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 0
fi

# 是否使用站点压力控制功能
rm -f ${status_dir}/task_list_*
if [ "${switch_use_compress_control}" = "y" -o "${switch_use_compress_control}" = "Y" ]; then
  url_input_dir=${selector_input_dir}
  #if [ "${updater_input_dir} " ]; then
  #   if [ "${url_input_dir}" ]; then
  #     url_input_dir=${url_input_dir},${updater_input_dir}
  #   else
  #     url_input_dir=${updater_input_dir}
  #   fi
  #fi
  source crawler/dispatcher/host_compress_control.sh
  ${HADOOP_HOME}/bin/hadoop fs -dus ${url_input_dir}/part* > ${status_dir}/total_input_parts
else
  rm -f ${status_dir}/total_input_parts &>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -dus ${selector_input_dir}/part* > ${status_dir}/total_input_parts
  #if [ "${updater_input_dir}" ]; then
  #  ${HADOOP_HOME}/bin/hadoop fs -dus ${updater_input_dir}/part* >> ${status_dir}/total_input_parts
  #fi
fi

if [ ! "${crawler_worker_monitor_dir}" -o ! "${crawler_worker_num}" ]; then
  err_msg="Error, crawler_worker_monitor_dir or crawler_worker_num in conf file is empty."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -test -d ${crawler_worker_monitor_dir} &>/dev/null
if [ $? -ne 0 ]; then
   ${HADOOP_HOME}/bin/hadoop fs -mkdir ${crawler_worker_monitor_dir}
   if [ $? -ne 0 ]; then
     err_msg= "Fail, hadoop fs -mkdir ${crawler_worker_monitor_dir}" 
     echo ${err_msg}
     SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
     ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
     exit 1
   fi
fi

cnt=0
total_file_size=0
while read line 
do
  hdfs_file=`echo ${line} | awk '{print $1}'`
  file_size=`echo ${line} | awk '{print $2}'`
  ## 忽略大小为0的空文件
  [ ${file_size} -eq 0 ] && continue
  ((total_file_size = total_file_size + file_size))
  ## 去掉路径前面的机器名和端口
  ## hdfs://h137:9000/user/pengdan/crawler/newlink/2012-01-08-134614/par1_00000
  hdfs_file_relative=`echo ${hdfs_file} | awk -F':' '{print $3}' | sed 's/^[0-9]*//'`
  tmp=`expr ${cnt} % ${crawler_worker_num}`
  echo ${hdfs_file_relative} >> ${status_dir}/task_list_${id_array[$tmp]}
  cnt=`expr $cnt + 1`
done < ${status_dir}/total_input_parts 

if [ ${cnt} -eq 0 ]; then
  warning_msg= "Warning, all files are 0 byte, there is no url need to crawl"
  echo ${warning_msg}
  SendAlertViaMail "${warning_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 0
fi

## 抓取的输出路径(page/link/css/image/newlink)
if [ ! "${crawler_output_pool_root}" ]; then
  err_msg="Error, parameter crawler_output_pool_root is empty."
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi

page_hdfs_output_dir=${crawler_output_pool_root}/page.hf
link_hdfs_output_dir=${crawler_output_pool_root}/link.st
css_hdfs_output_dir=${crawler_output_pool_root}/css.hf
image_hdfs_output_dir=${crawler_output_pool_root}/image.hf

page_hdfs_output_dir_vip=${crawler_output_pool_root}/page_vip.hf
link_hdfs_output_dir_vip=${crawler_output_pool_root}/link_vip.st
css_hdfs_output_dir_vip=${crawler_output_pool_root}/css_vip.hf
image_hdfs_output_dir_vip=${crawler_output_pool_root}/image_vip.hf

newlink_hdfs_output_dir=${crawler_output_pool_root}/newlink.st
update_hdfs_output_dir=${crawler_output_pool_root}/update.st

echo "page_hdfs_output_dir=${page_hdfs_output_dir}" > ${status_dir}/default_output_dir
echo "link_hdfs_output_dir=${link_hdfs_output_dir}" >> ${status_dir}/default_output_dir
echo "css_hdfs_output_dir=${css_hdfs_output_dir}" >> ${status_dir}/default_output_dir
echo "image_hdfs_output_dir=${image_hdfs_output_dir}" >> ${status_dir}/default_output_dir
echo "newlink_hdfs_output_dir=${newlink_hdfs_output_dir}" >> ${status_dir}/default_output_dir
echo "update_hdfs_output_dir=${update_hdfs_output_dir}" >> ${status_dir}/default_output_dir
echo "page_hdfs_output_dir_vip=${page_hdfs_output_dir_vip}" >> ${status_dir}/default_output_dir
echo "link_hdfs_output_dir_vip=${link_hdfs_output_dir_vip}" >> ${status_dir}/default_output_dir
echo "css_hdfs_output_dir_vip=${css_hdfs_output_dir_vip}" >> ${status_dir}/default_output_dir
echo "image_hdfs_output_dir_vip=${image_hdfs_output_dir_vip}" >> ${status_dir}/default_output_dir

timestamp=`date "+%F-%H%M%S"`
for i in ${id_array[@]} 
do
  if [ -e ${status_dir}/task_list_${i} ]; then
    ## 对抓取任务进行 shuf 处理
    shuf ${status_dir}/task_list_${i} > ${status_dir}/task_list_${i}_tmp && mv ${status_dir}/task_list_${i}_tmp ${status_dir}/task_list_${i}
    ${HADOOP_HOME}/bin/hadoop fs -put ${status_dir}/task_list_${i} ${crawler_worker_monitor_dir}/task_list_${i}.${timestamp}
    if [ $? -ne 0 ]; then
      err_msg="Fail, hadoop fs -put ${status_dir}/task_list_${i} ${crawler_worker_monitor_dir}/task_list_${i}.${timestamp}"
      echo ${err_msg}
      SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
      ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
      exit 1
    fi
  fi
done
${HADOOP_HOME}/bin/hadoop fs -rm ${crawler_worker_monitor_dir}/default_output_dir 
${HADOOP_HOME}/bin/hadoop fs -put ${status_dir}/default_output_dir ${crawler_worker_monitor_dir}/default_output_dir
if [ $? -ne 0 ]; then
  err_msg="Fail: hadoop fs -put ${status_dir}/default_output_dir ${crawler_worker_monitor_dir}/default_output_dir" 
  echo ${err_msg}
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 1
fi

mv ${status_dir}/selector_donelist.tmp ${status_dir}/selector_donelist
${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}

echo "Done!"
exit 0
