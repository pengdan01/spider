#!/bin/bash
set -u

if [ $# -ne 1 ]; then
  echo "Error, Usage: $0 conf_file"
  exit 1
fi
conf_file=$1
if [ ! -e ${conf_file} ]; then
  echo "Error, conf file[${conf_file}] not found"
  exit 1
fi
source ${conf_file}

## crawler_control.sh will combine all the modulel about crawler

## First, Start Selector to generate links need to crawle
if [ ${run_selector} = "y" -o ${run_selector} = "Y" ]; then
  bash -x crawler/selector/crawler_selector.sh crawler/selector/crawler_selector.conf \
  </dev/null &> selector.log&
  wait $!
  if [ $? -ne 0 ]; then
    echo "Error, crawler_selector.sh fail"
    exit 1
  fi
else
  echo "INFO, the process crawler_selector is skipped."
fi

## Second, Dispatch all the crawle tasks to download worker
if [ ${run_dispatcher} = "y" -o ${run_dispatcher} = "Y" ]; then
  bash -x crawler/dispatcher/crawler_dispatcher.sh crawler/dispatcher/crawler_dispatcher.conf \
  </dev/null &> dispatcher.log&
  wait $!
  if [ $? -ne 0 ]; then
    echo "Error, crawler_dispatcher.sh fail"
    exit 1
  fi
else
  echo "INFO, the process crawler_dispatcher is skipped."
fi

## Third, Start all the download worker to crawle from the web
## In the process, we use pssh tool to make all the download work concurrently.
if [ ${run_download_worker} = "y" -o ${run_download_worker} = "Y" ]; then
  cmd="cd ~/wly_crawler_online && bash -x crawler/task_distribute.sh crawler/dispatcher/crawler_deploy.conf"
  #${pssh_home_dir}/pssh -A -i -h hosts_info/crawler_host.txt -p 128 -O StrictHostKeyChecking=no -t 0 "${cmd}"
  ${pssh_home_dir}/pssh -i -h hosts_info/crawler_host.txt -p 128 -O StrictHostKeyChecking=no -t 0 "${cmd}"
  if [ $? -ne 0 ]; then
    echo "Error, pssh[${cmd}] fail, program exit with 1"
    exit 1
  fi
else
  echo "INFO, the process crawler_download_workder is skipped."
fi

## Now, we will wait for all the downloaders to finish their tasks
while [ true ]
do
  bash crawler/tools/info_collect.sh hosts_info/crawler_host.txt >tmp_info_collect 2>tmp_info_collect.err
  if [ $? -ne 0 ]; then
    echo "Error, collect status info from the downloader fail, you need to handle "
    exit 1
  fi
  total_num=`cat tmp_info_collect | wc -l`
  if [ $total_num -eq 3 ]; then
    echo "OK ,all the loaders has finished there tasks"
    break
  else
   echo "Get Info line: $total_num, still some downloaders not finished, sleep and wait..."
   sleep 60
  fi
done

rm -f tmp_info_collect*

## Fourth, Start Offline analyze 
## Fifth, Start link_merge
## The two process can be done simultaneously 
if [ ${run_link_merge} = "y" -o ${run_link_merge} = "Y" ]; then
  bash -x crawler/link_merge/link_merge.sh crawler/link_merge/link_merge.conf \
  </dev/null &>merge.log&
  pid1=$!
else
  echo "INFO, the process crawler_link_merge is skipped."
fi 

if [ ${run_offline_analyze} = "y" -o ${run_offline_analyze} = "Y" ]; then
  bash -x crawler/offline_analyze/offline_analyze.sh crawler/offline_analyze/offline_analyze.conf \
  </dev/null &> analyze.log&
  pid2=$!
else
  echo "INFO, the process crawler_offline_analyze is skipped."
fi

if [ ${run_link_merge} = "y" -o ${run_link_merge} = "Y" ]; then
  wait ${pid1}
  if [ $? -ne 0 ]; then
    echo "Error, link_merge fail"
    exit 1
  fi
fi
if [ ${run_offline_analyze} = "y" -o ${run_offline_analyze} = "Y" ]; then
  wait ${pid2}
  if [ $? -ne 0 ]; then
    echo "Error, offline_analyze fail"
    exit 1
  fi
fi

echo "Done!"
exit 0
