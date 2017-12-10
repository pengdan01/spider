#!/bin/bash
set -u

shard_num=64
source_type=image

output="/tmp/pengdan/image/1.hf"

padflag=""
if [ $shard_num -lt 100 ]; then
  padflag=0
fi

((shard_num--))
for i in `seq -w 0 ${shard_num}`
do
  batch_output=/app/crawler/batch/${source_type}/shard_$i/2012-03-16-101135.hf
  ${HADOOP_HOME}/bin/hadoop fs -mkdir ${batch_output}
  ${HADOOP_HOME}/bin/hadoop fs -mv ${output}/${padflag}$i* ${batch_output}
  if [ $? -ne 0 ]; then
    echo "Fail: /hadoop fs -mv ${output}/${padflag}$i* ${batch_output}"
    exit 1
  fi
  ## Append Donelist
  rm -f donefile.tmp &>/dev/null
  done_file=/app/crawler/batch/${source_type}/shard_$i/donefile
  ${HADOOP_HOME}/bin/hadoop fs -get $done_file donefile.tmp
  if [ $? -ne 0 ]; then
    echo "Fail: hadoop fs -get $done_file donefile.tmp"
    exit 1
  fi
  old=`cat donefile.tmp | awk -F'\t' '{print $1}'`
  ${HADOOP_HOME}/bin/hadoop fs -mv ${old} ${old}_TO_DELETE
  if [ $? -ne 0 ]; then
    echo "Fail: hadoop fs -mv ${old} ${old}_TO_DELETE"
    exit 1
  fi
  echo -e "${batch_output}\t2012-03-16-101135" > donefile.tmp
   ${HADOOP_HOME}/bin/hadoop fs -rm $done_file
   ${HADOOP_HOME}/bin/hadoop fs -put donefile.tmp $done_file
  rm -f donefile.tmp &>/dev/null
done

echo "Done!"
exit 0
