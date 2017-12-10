#!/bin/bash
set -u

input=/user/pengdan/crawler/page/2011-12-20-210036.hf
output="/tmp/pengdan/debug_page/0.st"

mapper_cmd=".build/opt/targets/crawler/tools/handle_page_linkbase/mr_hfile_debug"
build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_map_cmd ${mapper_cmd} \
  --mr_input ${input} --mr_input_format hfile \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 512 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 0 \
  --mr_job_name "hfile_debug"
if [ $? -ne 0 ]; then
  echo "Error, hadoop job fail"
  exit 1
fi

exit 0
