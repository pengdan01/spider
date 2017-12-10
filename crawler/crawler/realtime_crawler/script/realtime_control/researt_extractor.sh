#!/bin/bash

pssh/pssh -h host_ip/extractor_host_ip -i -t 0 "cd ~/realtime_extractor/; bash -x shell/stop.sh"

sleep 3

pssh/pssh -h host_ip/extractor_host_ip -i -t 0 "cd ~/realtime_extractor/; nohup bash -x shell/start.sh &>realtime_extractor.log &"

exit 0
