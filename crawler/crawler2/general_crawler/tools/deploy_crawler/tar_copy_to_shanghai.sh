#!/bin/bash
bash -x deploy_scheduler.sh 
[ $? -ne 0 ] && exit 1

bash -x  deploy_crawler_css_extractor.sh 
[ $? -ne 0 ] && exit 1

rm -f crawl.tar.gz
tar -czvf crawl.tar.gz clean_restart.sh  crawler  css_fetcher  extractor  scheduler 
[ $? -ne 0 ] && exit 1

rsync -cavz crawl.tar.gz crawler@m04:~/
[ $? -ne 0 ] && exit 1

exit 0
