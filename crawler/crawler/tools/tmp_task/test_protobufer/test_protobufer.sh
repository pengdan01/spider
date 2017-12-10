#!/bin/bash
set -u

output_root_dir=/tmp/pengdan

input=/tmp/pengdan/test.sf/part0
timestamp=`date "+%F-%H%M%S"`
output="${output_root_dir}/${timestamp}.hf"

mapper_cmd="mapper_test_protobuf --logtostderr"
reducer_cmd="reducer_test_protobuf --logtostderr"
mapreduce/submit.py \
  --ignore_input_error \
  --mr_map_cmd ${mapper_cmd} \
  --mr_reduce_cmd ${reducer_cmd} \
  --mr_cache_archives /app/crawler/app_data/web.tar.gz#web \
  --mr_input ${input} --mr_input_format sfile \
  --mr_output ${output} --mr_output_format hfile \
  --mr_min_split_size 1024 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 20 \
  --mr_job_name "convert_sf_to_hf"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi

echo "Done, output to hdfs: ${output}"
exit 0
