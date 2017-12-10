#!/bin/bash

pssh/pssh -h host_ip/crawl_host_ip -i -t 0 "cd ~/realtime_crawler/; bash -x shell/stop.sh"

pssh/pssh -h host_ip/job_queue_20080 -i -t 0 "cd ~/job_queue; bash -x stop_job_queue.sh 20080"
pssh/pssh -h host_ip/job_queue_20080 -i -t 0 "cd ~/job_queue; nohup bash -x start_job_queue.sh 20080 &>20080.log&"

pssh/pssh -h host_ip/crawl_host_ip -i -t 0 "cd ~/realtime_crawler/; nohup bash -x shell/start.sh &>realtime_crawler.log &"

exit 0
