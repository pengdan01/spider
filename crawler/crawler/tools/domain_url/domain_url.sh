#!/bin/bash
set -u

[ ! -d ${status_dir} ] && mkdir -p ${status_dir}

output_root_dir=/user/pengdan/crawler_online/360_domain_url

## 获取 Domain Set 文件
domain_set=/app/crawler/domain_url/input.st/ajax_url.txt
${HADOOP_HOME}/bin/hadoop fs -get ${domain_set} ${domain_filepath}.tmp
if [ $? -ne 0 ]; then
  echo "Get Domain Set fail,input: $input"
  exit 1
fi

if [ -s ${status_dir}/${domain_filepath} ]; then
  diff ${status_dir}/${domain_filepath} ${status_dir}/${domain_filepath}.tmp 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "The task has no diff from last domain set, no new task need to done!"
    exit 0
  fi
fi

${HADOOP_HOME}/bin/hadoop fs -test -e ${linkbase_donelist}
if [ $? -ne 0 ]; then
  echo "Error, ${linkbase_donelist} not exist"
  exit 1
fi

rm -f ${status_dir}/linkbase_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -get ${linkbase_donelist} ${status_dir}/linkbase_donelist.tmp
if [ $? -ne 0 ]; then
  echo "Fail: hadoop fs -get ${linkbase_donelist} ${status_dir}/linkbase_donelist.tmp"
  exit 1
fi

input=`cat ${status_dir}/linkbase_donelist.tmp | tail -1 | awk '{print $1}'`
timestamp=`date "+%F-%H%M%S"`
output="${output_root_dir}/${timestamp}.st"

mapper_cmd="mr_domain_url_mapper --domain_filepath=${domain_filepath} --logtostderr"
reducer_cmd="mr_domain_url_reducer --logtostderr"
mapreduce/submit.py \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_input ${input} --mr_input_format streaming_text \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_cache_archives /app/crawler/app_data/crawler.tar.gz#crawler \
  --mr_files ${status_dir}/${domain_filepath}.tmp \
  --mr_min_split_size 1024 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 10 \
  --mr_job_name "360_domain_url"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi

mv ${status_dir}/${domain_filepath}.tmp ${status_dir}/${domain_filepath}
mv ${status_dir}/linkbase_donelist.tmp ${status_dir}/linkbase_donelist

## Append donelist
record="${output}\t${timestamp}"
${HADOOP_HOME}/bin/hadoop fs -test -e ${domain_url_donelist}
if [ $? -eq 0 ]; then
  rm -f domain_url_donelist.tmp &>/dev/null
  ${HADOOP_HOME}/bin/hadoop fs -get ${domain_url_donelist} domain_url_donelist.tmp
  if [ $? -ne 0 ]; then
    echo "Failed: hadoop fs -get ${domain_url_donelist} domain_url_donelist.tmp"
    exit 1
  fi
fi
echo -e ${record} >> domain_url_donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -rm ${domain_url_donelist}
${HADOOP_HOME}/bin/hadoop fs -put domain_url_donelist.tmp ${domain_url_donelist}
if [ $? -ne 0 ]; then
  echo "Failed: hadoop fs -put domain_url_donelist.tmp ${domain_url_donelist}"
  exit 1
fi
echo "Done, output to hdfs: ${output}"
exit 0
