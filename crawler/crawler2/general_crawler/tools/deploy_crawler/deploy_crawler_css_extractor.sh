#!/bin/bash

# 本脚本生成 crawler css_fetcher extractor 运行时目录.
# 注意: 有符号链接
# 1. data/bin_index_model: 指向 ../../../dict/bin_index_model
# 2. web:     指向 ../../../dict/web
# 3. nlp:     指向 ../../../dict/nlp
# 4. feature: 指向 ../../../dict/feature

### crawler
rm -rf crawler
# bin
mkdir -p crawler/bin && cp /home/pengdan/workplace/wly/.build/opt/targets/crawler2/general_crawler/crawler crawler/bin/
# conf 
mkdir -p crawler/conf && cp /home/pengdan/workplace/wly/crawler2/general_crawler/crawler.cfg crawler/conf/
# shell
mkdir -p crawler/shell && cp /home/pengdan/workplace/wly/crawler2/general_crawler/script/crawler/* crawler/shell/
# data
mkdir -p crawler/data  && \
cp /home/pengdan/workplace/wly/crawler2/general_crawler/data/crawler/* crawler/data/ && \
mkdir -p crawler/log && mkdir -p crawler/run && mkdir -p crawler/output
cd crawler && ln -s ../../dict/web web && cd -

### css_fetcher
rm -rf css_fetcher
# bin
mkdir -p css_fetcher/bin && cp /home/pengdan/workplace/wly/.build/opt/targets/crawler2/general_crawler/css_fetcher css_fetcher/bin/
# shell
mkdir -p css_fetcher/shell && cp /home/pengdan/workplace/wly/crawler2/general_crawler/script/css_fetcher/* css_fetcher/shell/
# data
mkdir -p css_fetcher/data  && \
cp /home/pengdan/workplace/wly/crawler2/general_crawler/data/css_fetcher/* css_fetcher/data/ && \
mkdir -p css_fetcher/log && mkdir -p css_fetcher/run
cd  css_fetcher && ln -s ../../dict/web web && cd -

### extractor 
rm -rf extractor
# bin
mkdir -p extractor/bin && cp /home/pengdan/workplace/wly/.build/opt/targets/crawler2/general_crawler/extractor extractor/bin/
# conf 
mkdir -p extractor/conf && cp /home/pengdan/workplace/wly/crawler2/general_crawler/extractor.cfg extractor/conf/
# shell
mkdir -p extractor/shell && cp /home/pengdan/workplace/wly/crawler2/general_crawler/script/extractor/* extractor/shell/
# data
mkdir -p extractor/data  && \
cp /home/pengdan/workplace/wly/crawler2/general_crawler/data/extractor/* extractor/data/ && \
mkdir -p extractor/log && mkdir -p extractor/run && mkdir -p extractor/output
cd extractor && ln -s ../../dict/web web && cd -

exit 0
