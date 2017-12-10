#!/bin/bash
[ $# -ne 1 ] && exit 1
target_url=$1

pssh/pssh -h host_ip/crawl_host_ip -i -t 0 "cat ~/realtime_crawler/log/realtime_crawler.INFO | grep $target_url"

pssh/pssh -h host_ip/extractor_host_ip -i -t 0 "cat ~/realtime_extractor/log/realtime_extractor.INFO | grep $target_url"

exit 0
