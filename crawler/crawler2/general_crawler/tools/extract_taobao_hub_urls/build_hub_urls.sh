#!/bin/bash

#读取文件
rm -f .hub_urls.txt
while read line
do
  curl "$line" -L -o .raw.html
  if [ $? -ne 0 ]; then
    echo "curl fail and ignore, url: $line"
    continue
  fi
  
  #2. 网页转码成 utf8
  cat .raw.html | grep -v '<!--' | iconv -f gbk -t utf8 > .list_utf8.html
  if [ $? -ne 0 ]; then
    echo "iconv fail, url: $line"
    continue
  fi
  
  #3. 调研链接提取 python 脚本 
  python extractor.py 
  if [ $? -ne 0 ]; then
    echo "python extractor.py fail, url: $line"
    continue
  fi
done < seed_url.txt

#4. 去除掉无用的链接
#cat .hub_urls.txt | awk -F'#' '{if(NF>=2 && index($2, "cat=") == 0) {print $1;} else {print $0;}}' | sort -u | shuf > hub_urls.txt
# 由于当前只能根据 '#!' 构造 JSON 格式的 url
cat .hub_urls.txt manual_extracted.txt | grep '#!' | sort -u | shuf > hub_urls.txt

url_num=`cat hub_urls.txt | wc -l`
echo "final hub urls:  $url_num"

exit 0
