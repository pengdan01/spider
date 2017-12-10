#!/bin/bash
set -u

source_type=image

input=/app/crawler/batch/${source_type}/shard_*/2012*.hf
output="/user/pengdan/tmp/link/image/"

mapper_cmd="mr_rebuild_linkbase_from_page_mapper --logtostderr"
mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_map_cmd ${mapper_cmd} \
  --mr_input ${input} --mr_input_format hfile \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 512 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 0 \
  --mr_job_name "Rebuild_linkbase"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi

exit 0
