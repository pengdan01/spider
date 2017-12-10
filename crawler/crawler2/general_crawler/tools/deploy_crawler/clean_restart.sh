#!/bin/bash
dir=`pwd`

## restart scheduler 
#cd ${dir}/scheduler
#bash -x shell/stop.sh
#rm -f log/*
#nohup bash -x shell/start_scheduler.sh &>scheduler.log &
#[ $? -ne 0 ] && exit -1

# restart crawler 
cd ${dir}/crawler
bash -x shell/stop.sh
rm -f log/*
rm -f output/*
nohup bash -x shell/start_crawler.sh &>crawler.log &
[ $? -ne 0 ] && exit -1

# restart css_fetcher
cd ${dir}/css_fetcher
bash -x shell/stop.sh
rm -f log/*
nohup bash -x shell/start_css_fetcher.sh &>css_fetcher.log &
[ $? -ne 0 ] && exit -1

# restart extractor 
cd ${dir}/extractor
bash -x shell/stop.sh
rm -f log/*
rm -f output/*
nohup bash -x shell/start_extractor.sh &>extractor.log &
[ $? -ne 0 ] && exit -1

echo "done!" 
exit 0
