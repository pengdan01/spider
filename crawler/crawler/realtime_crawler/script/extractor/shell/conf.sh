#!/bin/bash

pidfile=run/pid
lockfile=run/lock

alert_receiver=pengdan@oneboxtech.com

extractor_config_file=conf/realtime_extractor.cfg

binary=.bin/realtime_extractor
#params="--log_dir=log --config_path=conf/realtime_extractor.cfg --http_port_for_counters=8090 --ds_redis_ips_file data/redis_url_servers.txt"
params="--backup_mode=false --log_dir=log --config_path=conf/realtime_extractor.cfg --http_port_for_counters=8090 --ds_redis_ips_file data/redis_url_servers.txt"

interval_to_report=1800
time_to_restart=600
