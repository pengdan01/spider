#!/bin/bash
set -u

## 将网页 / CSS / Image 等文件 Move 到 delta 公共库
if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 move_to_delta_conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}
tmp_dir=~/crawler_status/mv_to_delta
[ ! -d ${tmp_dir} ] && mkdir -p ${tmp_dir}

if [ ! "${crawler_saver_donelist}" -o ! "${shard_num_info_hdfs_file}" ]; then
  echo "Error, var saver_donelist/shard_num_info_hdfs_file is empty string" 
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -test -e ${crawler_saver_donelist}
if [ $? -ne 0 ]; then
  echo "Error, Not found crawler_saver_donelist[${crawler_saver_donelist}] in hadoop"
  exit 1
fi
rm -f ${tmp_dir}/saver_donelist.tmp &>/dev/null
${HADOOP_HOME}/bin/hadoop fs -get ${crawler_saver_donelist} ${tmp_dir}/saver_donelist.tmp
if [ $? -ne 0 ]; then
  echo "Error, hadoop get crawler_saver_donelist[${crawler_saver_donelist}] fail"
  exit 1
fi
## 任务是否已经处理过，即：saver_donelist 没有更新
if [ -e ${tmp_dir}/saver_donelist ]; then
  diff ${tmp_dir}/saver_donelist ${tmp_dir}/saver_donelist.tmp &>/dev/null
  if [ $? -eq 0 ]; then
    echo "Warning, saver donelist not updated and no new task need to process"
    exit 0
  fi
fi
## 检查 page/css/image delta 库根目录是否存在
${HADOOP_HOME}/bin/hadoop fs -test -d ${page_delta_root_dir} &>/dev/null
ret1=$?
${HADOOP_HOME}/bin/hadoop fs -test -d ${css_delta_root_dir} &>/dev/null
ret2=$?
${HADOOP_HOME}/bin/hadoop fs -test -d ${image_delta_root_dir} &>/dev/null
ret3=$?
if [ $ret1 -ne 0 -o $ret2 -ne 0 -o $ret3 -ne 0 ]; then
  echo "page/css/image delta root dir not exist on hadoop"
  exit 1
fi
## 获取网页库的 shard number 等信息
${HADOOP_HOME}/bin/hadoop fs -test -e ${shard_num_info_hdfs_file}
if [ $? -ne 0 ]; then
  echo "Error, Not found shard_num_info_hdfs_file[${shard_num_info_hdfs_file}] in hadoop"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_hdfs_file} > ${tmp_dir}/shard_num_info
if [ $? -ne 0 ]; then
  echo "Error, Failed in: hadoop fs -cat ${shard_num_info_hdfs_file} > ${tmp_dir}/shard_num_info"
  exit 1
fi
page_shard_num=`cat ${tmp_dir}/shard_num_info | grep "^page_shard_num=" | awk -F'=' '{print $2}'`
css_shard_num=`cat ${tmp_dir}/shard_num_info | grep "^css_shard_num=" | awk -F'=' '{print $2}'`
image_shard_num=`cat ${tmp_dir}/shard_num_info | grep "^image_shard_num=" | awk -F'=' '{print $2}'`

## 从 saver_donelist 中获取最后一条记录，获取已经抓取的 page/css/image/update 的 hdfs 路径
saver_last_record=`tail -1 ${tmp_dir}/saver_donelist.tmp`
if [ ! "${saver_last_record}" ]; then
  echo "Error, hdfs file crawler_saver_donelist has nothing, get input data fail"
  exit 1
fi
## Take Case!!
page_hdfs_dir=`echo ${saver_last_record} | awk '{print $1}'`
css_hdfs_dir=`echo ${saver_last_record} | awk '{print $3}'`
image_hdfs_dir=`echo ${saver_last_record} | awk '{print $4}'`
## After analyze all those pages, now we need mv those pages to /app/crawler/delta
page_base_dir=`basename ${page_hdfs_dir}`
css_base_dir=`basename ${css_hdfs_dir}`
image_base_dir=`basename ${image_hdfs_dir}`

shard_prefix="shard_"
timestamp=`date "+%F-%H%M%S"`

## 开始 Move page/css/image/update 到 delta page/css/image/update 库
## move page && update info
((page_shard_num--))
for shard_id in `seq -w 0 ${page_shard_num}`
do
  target_hdfs_dir=${page_delta_root_dir}/${shard_prefix}${shard_id}/${page_base_dir}
  ${HADOOP_HOME}/bin/hadoop fs -test -d ${target_hdfs_dir} &>/dev/null
  if [ $? -ne 0 ]; then
    ${HADOOP_HOME}/bin/hadoop fs -mkdir ${target_hdfs_dir}
    if [ $? -ne 0 ]; then
      echo "Error, Failed to: hadoop fs -mkdir ${target_hdfs_dir}"
      exit 1
    fi
  fi 
  ${HADOOP_HOME}/bin/hadoop fs -mv ${page_hdfs_dir}/part_${shard_id}* ${target_hdfs_dir}
  if [ $? -ne 0 ]; then
    echo "Error, Failed to: hadoop fs -mv ${page_hdfs_dir}/part_${shard_id}* ${target_hdfs_dir}"
    exit 1
  fi
  ##Append Donelist to this delta shard
  cat /dev/null &> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -test -e ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -eq 0 ]; then
    rm -f ${tmp_dir}/donelist.tmp &>/dev/null
    ${HADOOP_HOME}/bin/hadoop fs -get ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist \
    ${tmp_dir}/donelist.tmp
    if [ $? -ne 0 ]; then
      echo "Error, Fail: hadoop fs -get ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist"
      exit 1
    fi
    ##这个目录是否已经加入到该 donelist ( 在任务失败后重启可能出现部分 shard 目录已经添加到 donelist 的情况)
    line_num=`cat ${tmp_dir}/donelist.tmp | grep "${target_hdfs_dir}" | wc -l`
    if [ $line_num -ne 0 ]; then
      echo "Warning, Shard id: ${shard_id}, not append donelist as it already contains it: ${target_hdfs_dir}"
      continue
    fi
  fi
  echo -e "${target_hdfs_dir}\t${timestamp}" >> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -rm ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -put ${tmp_dir}/donelist.tmp \
  ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop put ${tmp_dir}/donelist.tmp"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rm ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -mv ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp \
  ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -mv ${page_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp"
    exit 1
  fi
done

## move css
((css_shard_num--))
for shard_id in `seq -w 0 ${css_shard_num}`
do
  target_hdfs_dir=${css_delta_root_dir}/${shard_prefix}${shard_id}/${css_base_dir}
  ${HADOOP_HOME}/bin/hadoop fs -test -d ${target_hdfs_dir} </dev/null &>/dev/null
  if [ $? -ne 0 ]; then
    ${HADOOP_HOME}/bin/hadoop fs -mkdir ${target_hdfs_dir}
    if [ $? -ne 0 ]; then
      echo "Error, Fail: hadoop fs -mkdir ${target_hdfs_dir}"
      exit 1
    fi
  fi 
  ${HADOOP_HOME}/bin/hadoop fs -mv ${css_hdfs_dir}/part_0${shard_id}* ${target_hdfs_dir}
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -mv ${css_hdfs_dir}/part_0${shard_id}* ${target_hdfs_dir}"
    exit 1
  fi
  ##Append Donelist to this delta shard
  cat /dev/null &> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -test -e ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -eq 0 ]; then
    rm -f ${tmp_dir}/donelist.tmp
    ${HADOOP_HOME}/bin/hadoop fs -get ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist \
    ${tmp_dir}/donelist.tmp
    if [ $? -ne 0 ]; then
      echo "Error, Fail: hadoop get ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist"
      exit 1
    fi
    ##这个目录是否已经加入到该 donelist ( 在任务失败后重启可能出现部分 shard 目录已经添加到 donelist 的情况)
    line_num=`cat ${tmp_dir}/donelist.tmp | grep "${target_hdfs_dir}" | wc -l`
    if [ $line_num -ne 0 ]; then
      echo "Warning, Shard id: ${shard_id}, not append donelist as it already contains it: ${target_hdfs_dir}"
      continue
    fi
  fi
  echo -e "${target_hdfs_dir}\t${timestamp}" >> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -rm ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -put ${tmp_dir}/donelist.tmp \
  ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -put ${tmp_dir}/donelist.tmp"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rm ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -mv ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp \
  ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop fs -mv ${css_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp"
    exit 1
  fi
done

## move image 
((image_shard_num--))
for shard_id in `seq -w 0 ${image_shard_num}`
do
  target_hdfs_dir=${image_delta_root_dir}/${shard_prefix}${shard_id}/${image_base_dir}
  ${HADOOP_HOME}/bin/hadoop fs -test -d ${target_hdfs_dir} </dev/null &>/dev/null
  if [ $? -ne 0 ]; then
    ${HADOOP_HOME}/bin/hadoop fs -mkdir ${target_hdfs_dir}
    if [ $? -ne 0 ]; then
      echo "Error, Failed to: hadoop fs -mkdir ${target_hdfs_dir}"
      exit 1
    fi
  fi 
  ${HADOOP_HOME}/bin/hadoop fs -mv ${image_hdfs_dir}/part_0${shard_id}* ${target_hdfs_dir}
  if [ $? -ne 0 ]; then
    echo "Error, Failed to: hadoop fs -mv ${image_hdfs_dir}/part_0${shard_id}* ${target_hdfs_dir}"
    exit 1
  fi
  ##Append Donelist to this delta shard
  cat /dev/null &> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -test -e ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -eq 0 ]; then
    rm -f ${tmp_dir}/donelist.tmp
    ${HADOOP_HOME}/bin/hadoop fs -get ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist \
    ${tmp_dir}/donelist.tmp
    if [ $? -ne 0 ]; then
      echo "Error, Fail: hadoop get ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist"
      exit 1
    fi
    ##这个目录是否已经加入到该 donelist ( 在任务失败后重启可能出现部分 shard 目录已经添加到 donelist 的情况)
    line_num=`cat ${tmp_dir}/donelist.tmp | grep "${target_hdfs_dir}" | wc -l`
    if [ $line_num -ne 0 ]; then
      echo "Warning, Shard id: ${shard_id}, not append donelist as it already contains it: ${target_hdfs_dir}"
      continue
    fi
  fi
  echo -e "${target_hdfs_dir}\t${timestamp}" >> ${tmp_dir}/donelist.tmp
  ${HADOOP_HOME}/bin/hadoop fs -rm ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -put ${tmp_dir}/donelist.tmp \
  ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop put ${tmp_dir}/donelist.tmp"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -rm ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist 2>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -mv ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp \
  ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist
  if [ $? -ne 0 ]; then
    echo "Error, Fail: hadoop mv ${image_delta_root_dir}/${shard_prefix}${shard_id}/donelist.tmp"
    exit 1
  fi
done
echo "Done, Finished in moving data to delta directory, now begin crawle statistics...."

## 开始统计本次抓取的质量
page_dir=${page_delta_root_dir}/${shard_prefix}*/${page_base_dir}
local_tmpfile_prefix=/home/pengdan/tmp/statistic_${timestamp}.result
bash ~/wly_crawler_online/build_tools/mapreduce/internal/host_statistics_full.sh ${page_dir} \
${local_tmpfile_prefix} 3000 100 1 &>/dev/null
if [ $? -ne 0 ]; then
  echo "Warning: Take it easy, Failed in host_statistics"
  exit 1
fi
receiver=pengdan@oneboxtech.com
title="[REPORT][`date +%D`][`date +%T`] crawle statistic"
host_html=${local_tmpfile_prefix}_host.htm
domain_html=${local_tmpfile_prefix}_domain.htm
path_html=${local_tmpfile_prefix}_path.htm
( uuencode ${host_html} ${host_html}; uuencode ${domain_html} ${domain_html}; \
uuencode ${path_html} ${path_html} ) | mail -s "$title" "${receiver}" 
if [ $? -ne 0 ]; then
  echo "Warning: Take it easy, Failed in send mail"
  exit 1
fi
exit 0
