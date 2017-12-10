#!/bin/bash
set -u

## 将 crawler .Trash 目录下的 Page | Css | Image 进行历史数据的清除

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 crawler_trash_clean_vip.conf"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}

if [ ! "${shard_num_info_hdfs_file}" -o ! "${delete_type}" -o ! "${keep_version_num}" ]; then
  echo "Error, var shard_num_info_hdfs_file/delete_type/keep_version/num is empty"
  exit 1
fi

${HADOOP_HOME}/bin/hadoop fs -test -e ${shard_num_info_hdfs_file}
if [ $? -ne 0 ]; then
  echo "Error, Not found shard_num_info_hdfs_file[${shard_num_info_hdfs_file}] in hadoop"
  exit 1
fi
## 获取 shard num
page_shard_num=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_hdfs_file} | grep "^page_shard_num=" \
| awk -F'=' '{print $2}'`
if [ ! "${page_shard_num}" ]; then
  echo "Error, Failed to get shard_num from hdfs file ${shard_num_info_hdfs_file}"
  exit 1
fi
css_shard_num=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_hdfs_file} | grep "^css_shard_num=" \
| awk -F'=' '{print $2}'`
if [ ! "${css_shard_num}" ]; then
  echo "Error, Failed to get shard_num from hdfs file ${shard_num_info_hdfs_file}"
  exit 1
fi
image_shard_num=`${HADOOP_HOME}/bin/hadoop fs -cat ${shard_num_info_hdfs_file} | grep "^image_shard_num=" \
| awk -F'=' '{print $2}'`
if [ ! "${image_shard_num}" ]; then
  echo "Error, Failed to get shard_num from hdfs file ${shard_num_info_hdfs_file}"
  exit 1
fi

for i in ${delete_type}
do
  source_type=$i
  if [ $source_type != "page_vip" -a $source_type != "css_vip" -a $source_type != "image_vip" ]; then
    echo "Error, Invalid source type: ${source_type}, shoud be page | css | image VIP"
    exit 1
  fi
  if [ $source_type = "page_vip" ]; then
    shard_num=${page_shard_num}
  elif [ $source_type = "image_vip" ]; then
    shard_num=${css_shard_num}
  else
    shard_num=${css_shard_num}
  fi
  ((--shard_num))
  for j in `seq -w 0 ${shard_num}`
  do
    batch_target_dir=${trash_hdfs_root_dir}/batch/${source_type}/shard_$j
    delta_target_dir=${trash_hdfs_root_dir}/delta/${source_type}/shard_$j
    ${HADOOP_HOME}/bin/hadoop fs -ls ${batch_target_dir} | grep -v "^Found " 1>tmp.$source_type.$j
    line_num=`cat tmp.$source_type.$j | wc -l`
    if [ $line_num -gt ${keep_version_num} ]; then
      ((line_num -= keep_version_num))
      while read line
      do
        [ $line_num -le 0 ] && break
        delete_dir=`echo $line | awk '{print $NF}'`
        ${HADOOP_HOME}/bin/hadoop fs -rmr ${delete_dir}
        [ $? -ne 0 ] && echo "Failed in: /hadoop fs -rmr ${delete_dir}"
        ((line_num--))
      done < tmp.$source_type.$j
    fi
    ${HADOOP_HOME}/bin/hadoop fs -ls ${delta_target_dir} | grep -v "^Found " 1>tmp.$source_type.$j
    line_num=`cat tmp.$source_type.$j | wc -l`
    if [ $line_num -gt ${keep_version_num} ]; then
      ((line_num -= keep_version_num))
      while read line
      do
        [ $line_num -le 0 ] && break
        delete_dir=`echo $line | awk '{print $NF}'`
        ${HADOOP_HOME}/bin/hadoop fs -rmr ${delete_dir}
        [ $? -ne 0 ] && echo "Failed in: /hadoop fs -rmr ${delete_dir}"
        ((line_num--))
      done < tmp.$source_type.$j
      rm -f tmp.$source_type.$j
    fi
  done
done

echo "Done!"
exit 0
