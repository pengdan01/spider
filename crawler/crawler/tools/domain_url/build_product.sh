#!/bin/bash

set -u

rm -rf ./domain_url_pro
rm -rf ./domain_url_pro.tar.gz

mkdir -p ./domain_url_pro

path=`pwd`
cd /home/pengdan/workplace/wly

./gen_makefile.sh crawler/tools/domain_url/BUILD
[ $? -ne 0 ] && echo "gen_makefile.sh fail" && exit 1

make clean
make opt -j8
[ $? -ne 0 ] && echo "make opt fail" && exit 1

cp .build/opt/targets/crawler/tools/domain_url/mr_domain_url_* ${path}/domain_url_pro/
[ $? -ne 0 ] && echo "cp binary code fail" && exit 1

cp -r build_tools/mapreduce ${path}/domain_url_pro/
[ $? -ne 0 ] && echo "cp build_tools fail" && exit 1

cd ${path}

cp domain_url.sh domain_url_pro/
[ $? -ne 0 ] && echo "cp domain_url.sh fail" && exit 1

tar -czvf domain_url_pro.tar.gz domain_url_pro/*
scp -r domain_url_pro.tar.gz 180.153.227.159:~/tmp/

exit 0


