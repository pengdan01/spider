#!/bin/bash

# 1. 获取京东全部商品总 list 页面
curl 'http://www.suning.com/emall/SNProductCatgroupView?storeId=10052&catalogId=10051&flag=1' -L -o .suning.html
[ $? -ne 0 ] && exit 1

##2. 网页转码成 utf8
#iconv -f gbk -t utf8 .suning.html > .suning_list_utf8.html
#[ $? -ne 0 ] && exit 1
mv .suning.html .suning_list_utf8.html

#3. 调研链接提取 python 脚本 
python suning_extractor.py 
[ $? -ne 0 ] && exit 1

#4. 去除掉无用的链接
cat .hub_urls.txt | sort -u > hub_urls.txt

echo "total hub urls(unique): `cat hub_urls.txt | wc -l`"
exit 0
