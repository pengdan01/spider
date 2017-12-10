#!/bin/bash

make clean

./gen_makefile.sh crawler/tools/handle_page_linkbase/BUILD
if [ $? -ne 0 ]; then
  echo "gen_makefile.s fail"
  exit 1
fi

make opt -j8
if [ $? -ne 0 ]; then
  echo "make fail"
  exit 1
fi

rm -rf handle_page_linkbase &>/dev/null
rm -rf handle_page_linkbase.tar.gz &>/dev/null
mkdir -p handle_page_linkbase
if [ $? -ne 0 ]; then
  exit 1
fi

cp -r build_tools/mapreduce handle_page_linkbase/
if [ $? -ne 0 ]; then
  exit 1
fi

cp crawler/tools/handle_page_linkbase/*.sh handle_page_linkbase/
if [ $? -ne 0 ]; then
  exit 1
fi

cp .build/opt/targets/crawler/tools/handle_page_linkbase/* handle_page_linkbase/
if [ $? -ne 0 ]; then
  exit 1
fi

tar -czvf handle_page_linkbase.tar.gz handle_page_linkbase/*
if [ $? -ne 0 ]; then
  exit 1
fi

scp handle_page_linkbase.tar.gz 180.153.227.26:~/tmp/
if [ $? -ne 0 ]; then
  echo "scp fail"
  exit 1
fi
exit 0
