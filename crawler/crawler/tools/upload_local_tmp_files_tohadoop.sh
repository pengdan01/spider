#!/bin/bash
set -u

## 脚本将 cralwer 抓取时缓存在本地 tmp 目录下的所有 page/css/image 文件上传到 
## Hadoop。由于在 crawler 正常结束时会自动上传并删除这些临时文件。所以该脚本只有
## 在 crawler 非正常推出时才使用该脚本上次本次抓取的临时换成结果。

pssh_home_dir=wly_crawler_online/build_tools/pssh
output_tagfile=~/crawler_status/fetcher/done_tag_*

cat ${output_tagfile} 2>/dev/null 1>hdfs_dir.file
if [ ! -s hdfs_dir.file ]; then
  echo "Warning, file[${output_tagfile}] not matched"
  exit 0
fi
worker_id=`ls -al ${output_tagfile} | tail -1 | awk -F'_' '{print $NF}' `
if [ ! "${worker_id}" ]; then
  echo "Warning, get workd_id fail, skip this worker"
  exit 0
fi
tmp_files_dir=~/wly_crawler_online/tmp_${worker_id}/saver

page_hdfs_dir=`cat hdfs_dir.file | grep "^page_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`
page_hdfs_dir_vip=`cat hdfs_dir.file | grep "^page_hdfs_output_dir_vip=" | tail -1 | awk -F'=' '{print $2}'`
link_hdfs_dir=`cat hdfs_dir.file | grep "^link_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`
link_hdfs_dir_vip=`cat hdfs_dir.file | grep "^link_hdfs_output_dir_vip=" | tail -1 | awk -F'=' '{print $2}'`
css_hdfs_dir=`cat hdfs_dir.file | grep "^css_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`
css_hdfs_dir_vip=`cat hdfs_dir.file | grep "^css_hdfs_output_dir_vip=" | tail -1 | awk -F'=' '{print $2}'`
image_hdfs_dir=`cat hdfs_dir.file | grep "^image_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`
image_hdfs_dir_vip=`cat hdfs_dir.file | grep "^image_hdfs_output_dir_vip=" | tail -1 | awk -F'=' '{print $2}'`
update_hdfs_dir=`cat hdfs_dir.file | grep "^update_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`
newlink_hdfs_dir=`cat hdfs_dir.file | grep "^newlink_hdfs_output_dir=" | tail -1 | awk -F'=' '{print $2}'`

rm -f hdfs_dir.file
update_files="${tmp_files_dir}/update_*"
for i in ${update_files}
do
  if [ -s ${i} ]; then
    file=`echo ${i} | awk -F'/' '{print $NF}'`
    ${HADOOP_HOME}/bin/hadoop fs -rm ${update_hdfs_dir}/${file} &>/dev/null
    ${HADOOP_HOME}/bin/hadoop fs -put ${i} ${update_hdfs_dir}
    if [ $? -ne 0 ]; then
      echo "Error, failed to hadoop put ${i} ${update_hdfs_dir}"
    fi
  fi
  rm -f ${i}
done &

newlink_files="${tmp_files_dir}/newlink_*"
for i in ${newlink_files}
do
  if [ -s ${i} ]; then
    file=`echo ${i} | awk -F'/' '{print $NF}'`
    ${HADOOP_HOME}/bin/hadoop fs -rm ${newlink_hdfs_dir}/${file} &>/dev/null
    ${HADOOP_HOME}/bin/hadoop fs -put ${i} ${newlink_hdfs_dir}
    if [ $? -ne 0 ]; then
      echo "Error, failed to hadoop put ${i} ${newlink_hdfs_dir}"
    fi
  fi
  rm -f ${i}
done &

page_tmp_files="${tmp_files_dir}/page*"
for i in ${page_tmp_files}
do
  linkfile=${tmp_files_dir}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${page_hdfs_dir} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

page_tmp_files_vip="${tmp_files_dir}/page_vip_*"
for i in ${page_tmp_files_vip}
do
  linkfile=${tmp_files_dir}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${page_hdfs_dir_vip} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir_vip}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir_vip}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

css_tmp_files="${tmp_files_dir}/css*"
for i in ${css_tmp_files}
do
  linkfile=${tmp_files_dir}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${css_hdfs_dir} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

css_tmp_files_vip="${tmp_files_dir}/css_vip_*"
for i in ${css_tmp_files_vip}
do
  linkfile=${tmp_files_dir_vip}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${css_hdfs_dir_vip} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir_vip}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir_vip}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

image_tmp_files="${tmp_files_dir}/image*"
for i in ${image_tmp_files}
do
  linkfile=${tmp_files_dir}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${image_hdfs_dir} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

image_tmp_files_vip="${tmp_files_dir}/image_vip_*"
for i in ${image_tmp_files_vip}
do
  linkfile=${tmp_files_dir}/link`echo ${i} | awk -F'/' '{print $NF}'`
  if [ -s ${i} ]; then
    .build/opt/targets/crawler/fetcher/file_upload --hdfs_output_dir ${image_hdfs_dir_vip} \
    --tmp_file ${i}
    if [ $? -eq 0 ]; then
      ${HADOOP_HOME}/bin/hadoop fs -put ${linkfile} ${link_hdfs_dir_vip}
      if [ $? -ne 0 ]; then
        echo "Error, failed to hadoop put ${linkfile} ${link_hdfs_dir_vip}"
      fi
    fi
  fi
  rm -f ${i} ${linkfile}
done &

wait

## Now delete all log files
rm -f file_upload*
rm -f core.*

echo "Upload Done!"
exit 0
