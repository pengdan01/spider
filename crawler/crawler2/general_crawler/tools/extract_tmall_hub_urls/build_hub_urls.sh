#!/bin/bash

# 1. 获取京东全部商品总 list 页面
curl http://www.tmall.com/ -L -o .raw.html
[ $? -ne 0 ] && exit 1

#2. 网页转码成 utf8
iconv -f gbk -t utf8 .raw.html > .list_utf8.html
[ $? -ne 0 ] && exit 1

#3. 调研链接提取 python 脚本 
python extractor.py 
[ $? -ne 0 ] && exit 1

#4. 去除掉无用的链接
cat .hub_urls.txt | sort -u | shuf > hub_urls.txt

url_num=`cat hub_urls.txt | wc -l`
echo "final hub urls:  $url_num"

exit 0
