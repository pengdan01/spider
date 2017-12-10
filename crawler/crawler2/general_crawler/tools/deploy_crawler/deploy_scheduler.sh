#!/bin/bash

# 本脚本生成 scheduler 运行时目录.
# 注意: 有两个符号链接
# 1. data/bin_index_model: 指向 ../../../dict/bin_index_model
# 2. web: 指向 ../../../dict/web

rm -rf scheduler

# bin
mkdir -p scheduler/bin && cp /home/pengdan/workplace/wly/.build/opt/targets/crawler2/general_crawler/scheduler scheduler/bin/

# shell
mkdir -p scheduler/shell && cp /home/pengdan/workplace/wly/crawler2/general_crawler/script/scheduler/* scheduler/shell/

# data
mkdir -p scheduler/data  && \
cp /home/pengdan/workplace/wly/crawler2/general_crawler/data/scheduler/* scheduler/data/ && \
cd scheduler/data && ln -s  ../../../dict/bin_index_model bin_index_model && cd -

mkdir -p scheduler/log && mkdir -p scheduler/run

cd scheduler && ln -s ../../dict/web web

exit 0

