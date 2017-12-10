#!/bin/bash

[ $# -ne 1 ] && echo "Usage: $0 host_list_file" && exit 1

host_list_file=$1

[ ! -f ${host_list_file} ] && echo "host list file[${host_list_file}] not exist" && exit 1

## kill all fetcher2 and bash runing on each node 
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 'pkill -f fetcher_control'
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 'pkill -f fetcher2'
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 'pkill -f task_distribute'
while true;do
  uploading=`build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 'ps ux | grep upload | grep -v grep' | grep upload`
  if [[ "${uploading}" != "" ]];then
    sleep 10
    continue
  fi
  break
done

## Delete the old fetcher2 dir
build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "rm -rf /home/crawler/fetcher2"
if [ $? -ne 0 ]; then
  echo "Failed: pssh -h ${host_list_file} -t 0 'rm -rf /home/crawler/fetcher2'"
  exit 1
fi

## cp fetcher2 to each node 
## XXX(pengdan): Random the fetcher id, so all the available ips can be used.
#cat fetcher2/c_crawler_deploy_base.conf | grep -v "^#" | grep -v "^$" | cut -d' ' -f1 > host
#cat fetcher2/c_crawler_deploy_base.conf | grep -v "^#" | grep -v "^$" | cut -d' ' -f2 | shuf > id 
#paste host id -d' ' > fetcher2/c_crawler_deploy.conf

build_tools/pssh/pscp -h ${host_list_file} -r fetcher2 /home/crawler/
if [ $? -ne 0 ]; then
  echo "Failed: pscp -h ${host_list_file} -r fetcher2 /home/crawler/"
  exit 1
fi
rm -f host id

## every time, use the latest url norm dict 
url_norm_dict=/wly/web.tar.gz
local_web_tar=web.tar.gz
rm -f ${local_web_tar}
${HADOOP_HOME}/bin/hadoop fs -get ${url_norm_dict} ${local_web_tar}
[ $? -ne 0 ] && exit 1
[ -d web ] && rm -rf web 
mkdir -p web
cd web && tar -xzvf ../${local_web_tar}
cd ..

build_tools/pssh/pssh -h ${host_list_file} -t 0 -p 128 "rm -rf /home/crawler/web"
if [ $? -ne 0 ]; then
  echo "Failed: pssh -h ${host_list_file} -t 0 'rm -rf /home/crawler/web'"
  exit 1
fi
build_tools/pssh/pscp -h ${host_list_file} -r web /home/crawler/
if [ $? -ne 0 ]; then
  echo "Failed: pscp -h ${host_list_file} -r web /home/crawler/" 
  exit 1
fi

echo "done!"
exit 0
