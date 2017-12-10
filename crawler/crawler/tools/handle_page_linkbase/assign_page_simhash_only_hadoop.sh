#!/bin/bash

set -u

output_root_dir="/tmp/pengdan/page/shard_"
white_list=white_listfile
black_list=black_listfile
start_shard_id=043
end_shard_id=070
timestamp=`date "+%F-%H%M%S"`
mapper_cmd="mr_assign_page_simhash_mapper --white_list=${white_list} --black_list=${black_list} --logtostderr"
for i in `seq -w ${start_shard_id} ${end_shard_id}`
do
  input=/app/crawler/delta/page/shard_$i/2012-04-12*.hf
  tmp_output=${output_root_dir}$i/_tmp_${timestamp}.hf
  output=${output_root_dir}$i/${timestamp}.hf
  build_tools/mapreduce/submit.py \
    --mr_ignore_input_error \
    --mr_multi_output \
    --mr_slow_start_ratio 0.95 \
    --mr_map_cmd ${mapper_cmd} \
    --mr_input ${input} --mr_input_format hfile \
    --mr_output ${tmp_output} --mr_output_format hfile \
    --mr_cache_files /app/crawler/app_data/map_dict_from_doc_freq_tl_10_df_300#$white_list,/app/crawler/app_data/stopwords#$black_list \
    --mr_cache_archives /app/feature/dict_data/nlp.tar.gz#nlp \
    --mr_min_split_size 1024 \
    --mr_map_capacity 100 \
    --mr_reduce_tasks 0 \
    --mr_job_name "Calculate_simhash_delta.$i"
  if [ $? -ne 0 ]; then
    echo "Error, hadoop job fail"
    exit 1
  else
    ${HADOOP_HOME}/bin/hadoop fs -mv ${tmp_output} ${output}
    if [ $? -ne 0 ]; then
      echo "Fail:doop fs -mv ${tmp_output} ${output} "
      exit 1
    fi
  fi
done

exit 0
