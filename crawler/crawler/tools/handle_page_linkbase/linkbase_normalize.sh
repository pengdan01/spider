#!/bin/bash
set -u

mapfiles="/tmp/pengdan/2012-03-16-101135.hf/map*"
linkbase=/app/crawler/linkbase/2012-04-14-141412.st

output=/tmp/pengdan/linkbase/2012-04-16-181904.st

mapper_cmd="mr_normalize_linkbase_mapper --logtostderr"
reducer_cmd="mr_normalize_linkbase_reducer --logtostderr"
mapreduce/submit.py \
  --mr_slow_start_ratio 0.95 \
  --mr_map_cmd ${mapper_cmd} \
  --mr_cache_archives /app/crawler/app_data/crawler.tar.gz#crawler \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_input ${mapfiles}:hfile ${linkbase}:streaming_text --mr_input_format mixed \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 512 \
  --mr_reduce_tasks 10 \
  --mr_job_name "Normalize Linkbase"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi

exit 0

batch_linkdir=/app/crawler/linkbase/2012-04-16-181904.st
${HADOOP_HOME}/bin/hadoop fs -mkdir ${batch_linkdir}
if [ $? -ne 0 ]; then
  echo "Error, hadoop fs -mkdir ${batch_linkdir}"
  exit 1
fi

${HADOOP_HOME}/bin/hadoop fs -mv ${output}/part* ${batch_linkdir}
if [ $? -ne 0 ]; then
  echo "Error, hadoop fs -mv ${output}/part* ${batch_linkdir}"
  exit 1
fi

rm -rf donelist.tmp &> /dev/null
${HADOOP_HOME}/bin/hadoop fs -get /app/crawler/linkbase/link_merge_donelist donelist.tmp 
if [ $? -ne 0 ]; then
  echo "Error, hadoop fs -get /app/crawler/linkbase/link_merge_donelist donelist.tmp"
  exit 1
fi

echo -e "${batch_linkdir}\t2012-04-16-181904" >> donelist.tmp
${HADOOP_HOME}/bin/hadoop fs -rm /app/crawler/linkbase/link_merge_donelist 
${HADOOP_HOME}/bin/hadoop fs -put donelist.tmp /app/crawler/linkbase/link_merge_donelist 
if [ $? -ne 0 ]; then
  echo "Error, hadoop fs -put donelist.tmp /app/crawler/linkbase/link_merge_donelist"
  exit 1
fi
echo "Done!"
exit 0
