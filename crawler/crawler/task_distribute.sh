#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 crawler_deploy_conf_file"
  exit 1
fi
conf=$1
if [ ! -e ${conf} ]; then
  echo "Error, Conf file[${conf}] not exist"
  exit 1
fi

local_hostname=`hostname`
matched_line_num=`grep "^${local_hostname}" ${conf} | wc -l | awk '{print $1}'`
if [ ${matched_line_num} -eq 0 ]; then
  echo "Warning, Host: ${local_hostname}, not assigned for task."
  exit 0 
fi

worker_ids=(`grep "^${local_hostname}" ${conf} | awk '{print $2}'`)
rm -rf /dev/shm/crawler_worker*
for id in ${worker_ids[@]}
do
  bash -x crawler/fetcher/crawler_worker.sh crawler/fetcher/crawler_worker.conf \
  $id </dev/null &> /dev/shm/crawler_worker.$id & 
  pid[$id]=$!
done

## wait for complete
some_task_failed_flag=0
for id in ${pid[@]}
do
 wait ${id}
 if [ $? -ne 0 ]; then
   echo "Error, download workder fail, host: ${local_hostname}, pid: ${id}, this task will be igored."
   some_task_failed_flag=1
   continue 
 fi
done

if [ ${some_task_failed_flag} -eq 0 ]; then 
  echo "OK, host ${local_hostname} finished all the tasks."
else
  echo "Warning, host[${local_hostname}] finished part of tasks and some failed, check log for details."
fi
exit 0
