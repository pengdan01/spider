#!/bin/bash
set -u
receiver=pengdan@oneboxtech.com
output_root_dir=/tmp/pengdan/check_big_site_valid_page

input=/user/crawler/app/jingdong/nvzhuang/item_res/20121214/14/page.sf/resource.20121214133748.m04.dong.shgt.qihoo.net.30883.data
#input=/user/crawler/app/jingdong/nvzhuang/item_res/20121213/11/page.sf/resource.20121213103956.m04.dong.shgt.qihoo.net.24278.data

timestamp=`date "+%F-%H%M%S"`
output="${output_root_dir}/${timestamp}.st"

mapper_cmd="mr_check_valid_page_mapper"
~/control_workspace/build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_map_cmd ${mapper_cmd} \
  --mr_cache_archives /wly/web.tar.gz#web,/wly/nlp.tar.gz#nlp \
  --mr_input ${input} --mr_input_format sfile \
  --mr_output ${output} --mr_output_format streaming_text \
  --mr_min_split_size 1024 \
  --mr_map_capacity 1000 \
  --mr_reduce_tasks 0 \
  --mr_cache_archives /user/crawler/price_recg/data.tar.gz#data \
  --mr_job_name "check_res_comment_price"
if [ $? -ne 0 ]; then
  err_msg="check_big_site_valid_page: error, hadoop job fail"
  echo ${err_msg}
  exit 1
fi
echo "output: $output"
exit 0
