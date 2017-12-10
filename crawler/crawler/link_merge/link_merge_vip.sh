#!/bin/bash
if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 link_merge_conf_file_vip"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, config file[${conf_file}] not exist" 
  exit 1
fi
source ${conf_file}

flag="vip"

status_dir=~/crawler_status/link_merge_vip
[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

input=""
if [ ! "${crawler_saver_donelist}" -o ! "${link_merge_output_root_dir}" ]; then
  echo "Error, parameter crawler_saver_donelist or link_merge_output_root_dir is empty"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -test -e ${crawler_saver_donelist}
if [ $? -ne 0 ]; then
  echo "Error, Not found crawler_saver_donelist[${crawler_saver_donelist}] in hadoop"
  exit 1
fi
## 从 saver_donelist 中获取最后一条记录，并提取已经抓取的 link 的 hdfs 路径
rm -f ${status_dir}/saver_donelist_$flag.tmp &>/dev/null
${HADOOP_HOME}/bin/hadoop fs -get ${crawler_saver_donelist} ${status_dir}/saver_donelist_$flag.tmp
if [ $? -ne 0 ]; then
  echo "Error, /hadoop fs -get ${crawler_saver_donelist} ${status_dir}/saver_donelist_$flag.tmp"
  exit 1
fi
## saver donelist 是否更新
if [ -e ${status_dir}/saver_donelist_$flag ]; then
  diff  ${status_dir}/saver_donelist_$flag  ${status_dir}/saver_donelist_$flag.tmp &>/dev/null
  if [ $? -eq 0 ]; then
    echo "WARNING, Saver donist local cached is the same as the new one, So no new links need to merge!"
    exit 0
  fi
fi
## saver_donelist 记录格式为:
## page_dir \t link_dir \t css_dir \t image_dir \t newlink_dir \t  update_dir \t page_vip \t link_vip
## \t css_vip \t image_vip \t timestamp
if [ "$flag" = "normal" ]; then
  link_dir=`cat ${status_dir}/saver_donelist_$flag.tmp | tail -1 | awk '{print $2}'`
else
  link_dir=`cat ${status_dir}/saver_donelist_$flag.tmp | tail -1 | awk '{print $8}'`
fi
update_dir=`cat ${status_dir}/saver_donelist_$flag.tmp | tail -1 | awk '{print $6}'`
if [ ! "${link_dir}" -o ! "${update_dir}" ]; then
  echo "Error, link hdfs dir or update hdfs dir is empty"
  exit 1
fi
## 由于 |link_dir| 中的文件数目太多 (50W), 需要使用工具将这些碎小的文件进行合并, 减少输入文件数
${HADOOP_HOME}/bin/hadoop fs -ls ${link_dir} | grep -v 'items' | awk '{print $NF}' | shuf > filelist_$flag.tmp
if [ $? -ne 0 ]; then
  echo "Error, generate filelist_$flag.tmp fail"
  exit 1
fi
map_num=500
timestamp=`date "+%F-%H%M%S"`
output_path=/user/pengdan/combined_dir/${timestamp}_${flag}.st
## $0 input_file_list_file output_path map_num"
filelist_path=/tmp/pengdan/filelist_$flag.tmp
${HADOOP_HOME}/bin/hadoop fs -rm ${filelist_path}
${HADOOP_HOME}/bin/hadoop fs -put filelist_$flag.tmp ${filelist_path}
build_tools/mapreduce/combine_st.sh ${filelist_path} ${output_path} ${map_num} 
if [ $? -ne 0 ]; then
  echo "Fail: build_tools/mapreduce/combine_st.sh ${filelist_path} ${output_path} ${map_num}"
  exit 1
fi
rm -f filelist_$flag.tmp

link_dir=${output_path}

## 当输入一个空目录时, submit.py 处理会有问题, 所以: 判断 |update_dir| 是否为空
part_num=`${HADOOP_HOME}/bin/hadoop fs -ls ${update_dir}/part* 2>/dev/null | wc -l`
if [ ${part_num} -eq 0 ]; then
  input=${link_dir}
else
  input=${link_dir},${update_dir}
fi

## 从 link_merge_donelist 获取上一次 merge 的linkbase 输出路径，用作本次 merge 的输入
local_link_merge_donelist=${status_dir}/link_merge_donelist_$flag.tmp
${HADOOP_HOME}/bin/hadoop fs -test -e ${link_merge_donelist}
if [ $? -eq 0 ]; then
  rm -f ${local_link_merge_donelist} 
  ${HADOOP_HOME}/bin/hadoop fs -get ${link_merge_donelist} ${local_link_merge_donelist} 
  if [ $? -ne 0 ]; then
    echo "Error, hadoop get ${link_merge_donelist} fail"
    exit 1
  fi
  ## link_merge_donelist 记录格式：linkbase_dir \t timestamp
  last_linkbase=`cat ${local_link_merge_donelist} | tail -1 | awk '{print $1}'`
  if [ "${last_linkbase}" ]; then
    input="${input},${last_linkbase}"
  fi
fi
## 检查输出根目录是否存在，不存在，则先创建之
${HADOOP_HOME}/bin/hadoop fs -test -e ${link_merge_output_root_dir}
if [ $? -ne 0 ]; then
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${link_merge_output_root_dir}
  if [ $? -ne 0 ]; then
    echo "Error, Failed to hadoop fs -mkdir ${link_merge_output_root_dir}"
    exit 1
  fi
fi
output="${link_merge_output_root_dir}/${timestamp}.st"
tmp_output="${link_merge_output_root_dir}/_tmp_${timestamp}.st"
mapper=".build/opt/targets/crawler/link_merge/mr_link_merge_mapper --logtostderr"
reducer=".build/opt/targets/crawler/link_merge/mr_link_merge_reducer --logtostderr"
cache_archive_files="--mr_cache_archives /app/crawler/app_data/web.tar.gz#web"
build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  ${cache_archive_files} \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper} --mr_reduce_cmd ${reducer} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${tmp_output} --mr_output_format streaming_text \
  --mr_min_split_size 1024 --mr_reduce_capacity 1000 --mr_map_capacity 1000 \
  --mr_reduce_tasks ${reduce_task_num} \
  --mr_job_name "crawler_link_merge_$flag"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi
${HADOOP_HOME}/bin/hadoop fs -mv ${tmp_output} ${output}
if [ $? -ne 0 ]; then
  echo "Error, Fail: hadoop fs -mv ${tmp_output} ${output}"
  exit 1
fi
## merge OK, Now append a record to link_merge_donelist 
record="${output}\t${timestamp}"
if [ -e "${local_link_merge_donelist}" ]; then
  ${HADOOP_HOME}/bin/hadoop fs -rm ${link_merge_donelist}
fi  
echo -e ${record} >> ${local_link_merge_donelist} 
${HADOOP_HOME}/bin/hadoop fs -put ${local_link_merge_donelist} ${link_merge_donelist} 
if [ $? -ne 0 ]; then
  echo "Error, Failed to put ${local_link_merge_donelist} ${link_merge_donelist}"
  exit 1
fi

mv ${status_dir}/saver_donelist_$flag.tmp ${status_dir}/saver_donelist_$flag
rm -f ${local_link_merge_donelist}

echo "Done!"
exit 0
