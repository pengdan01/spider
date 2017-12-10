#!/bin/bash
set -u

output_dir="wly_crawler_online"

if [ -d ${output_dir} ]; then
  rm -rf ${output_dir}
else
  mkdir -p ${output_dir}
fi
rm -rf ${output_dir}.tar.gz

mkdir -p ${output_dir}/crawler
if [ $? -ne 0 ]; then
  echo "create dir ${output_dir}/crawler fail, check your privileges"
  exit 1
fi
make clean

### build selector
#./gen_makefile.sh crawler/selector/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/selector/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for selector fail"
#  exit 1
#fi
#
### build dispatcher
#./gen_makefile.sh crawler/dispatcher/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/dispatcher/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for dispatcher fail"
#  exit 1
#fi

## build fetcher
./gen_makefile.sh crawler/fetcher/BUILD
if [ $? -ne 0 ]; then
  echo "./gen_makefile.sh crawler/fetcher/BUILD fail"
  exit 1
fi
make -j8 opt 
if [ $? -ne 0 ]; then
  echo "make -j8 opt for fetcher fail"
  exit 1
fi

### build link_merge 
#./gen_makefile.sh crawler/link_merge/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/link_merge/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for link_merge fail"
#  exit 1
#fi
#
### build offline_analyze 
#./gen_makefile.sh crawler/offline_analyze/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/offline_analyze/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for offline_analyze fail"
#  exit 1
#fi
#
### build statistic
#./gen_makefile.sh crawler/statistic/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/statistic/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for statistic fail"
#  exit 1
#fi
### build tools/offline_site_stat 
#./gen_makefile.sh crawler/tools/offline_site_stat/BUILD
#if [ $? -ne 0 ]; then
#  echo "./gen_makefile.sh crawler/tools/offline_site_stat/BUILD fail"
#  exit 1
#fi
#make -j8 opt 
#if [ $? -ne 0 ]; then
#  echo "make -j8 opt for crawler/tools/offline_site_stat/BUILD fail"
#  exit 1
#fi
## build counters
./gen_makefile.sh samples/collect_counters/BUILD
if [ $? -ne 0 ]; then
  echo "./gen_makefile.sh samples/collect_counters/BUILD fail"
  exit 1
fi
make -j8 opt 
if [ $? -ne 0 ]; then
  echo "make -j8 opt for collect_counters fail"
  exit 1
fi

## now construct directory tree
#mkdir -p ${output_dir}/crawler/selector 
#cp crawler/selector/*.conf ${output_dir}/crawler/selector/
#cp crawler/selector/*.sh ${output_dir}/crawler/selector/
#
#mkdir -p ${output_dir}/crawler/dispatcher
#cp crawler/dispatcher/*.conf ${output_dir}/crawler/dispatcher/
#cp crawler/dispatcher/*.sh ${output_dir}/crawler/dispatcher/

mkdir -p ${output_dir}/crawler/fetcher
cp crawler/fetcher/*.sh ${output_dir}/crawler/fetcher/
cp crawler/fetcher/*.conf ${output_dir}/crawler/fetcher/
cp crawler/fetcher/country-ipv4.lst ${output_dir}/crawler/fetcher/
cp crawler/fetcher/VIPhost.dat ${output_dir}/crawler/fetcher/
cp crawler/fetcher/*.txt ${output_dir}/crawler/fetcher/

#mkdir -p ${output_dir}/crawler/link_merge
#cp crawler/link_merge/*.conf ${output_dir}/crawler/link_merge/
#cp crawler/link_merge/*.sh ${output_dir}/crawler/link_merge/
#
#mkdir -p ${output_dir}/crawler/offline_analyze
#cp crawler/offline_analyze/*.conf ${output_dir}/crawler/offline_analyze/
#cp crawler/offline_analyze/*.sh ${output_dir}/crawler/offline_analyze/
#
cp -r crawler/task_distribute.sh ${output_dir}/crawler/
cp -r crawler/crawler_control.sh ${output_dir}/crawler/
cp -r crawler/crawler_control.conf ${output_dir}/crawler/
#
#mkdir -p ${output_dir}/crawler/statistic
#cp crawler/statistic/*.conf ${output_dir}/crawler/statistic/
#cp crawler/statistic/*.sh ${output_dir}/crawler/statistic/

mkdir -p ${output_dir}/crawler/tools
cp -r crawler/tools/*.sh ${output_dir}/crawler/tools/
cp -r crawler/tools/offline_site_stat ${output_dir}/crawler/tools/
rm -f ${output_dir}/crawler/tools/offline_site_stat/*.h
rm -f ${output_dir}/crawler/tools/offline_site_stat/*.cc

mkdir -p ${output_dir}/web/url
cp web/url/tld.dat ${output_dir}/web/url/

# copy shell directory to output_dir
mkdir -p ${output_dir}/shell
cp shell/*.sh ${output_dir}/shell/

## copy the binary file to ${output_dir}
mkdir -p ${output_dir}/.build
cp -r .build/opt ${output_dir}/.build/
cp -r .build/pb ${output_dir}/.build/
rm -f ${output_dir}/.build/opt/targets/crawler/*/*test*

## tmp
cp -r .build/diag_dbg ${output_dir}/.build/

## copy segement data to wly_crawler_online only when data is updateed
#mkdir -p ${output_dir}/nlp/segment/model
#cp -r pub/src/nlp/segment/model/segment_*  ${output_dir}/nlp/segment/model/
#if [ $? -ne 0 ]; then
#  exit 1
#fi

## copy necessary build tools 
mkdir -p ${output_dir}/build_tools
cp -r build_tools/mapreduce ${output_dir}/build_tools/
cp -r build_tools/pssh ${output_dir}/build_tools/

tar czvf ${output_dir}.tar.gz ${output_dir}/* ${output_dir}/.build

echo "done!"
exit 0
