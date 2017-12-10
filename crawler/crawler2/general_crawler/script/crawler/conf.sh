#!/bin/bash

pidfile=run/pid
lockfile=run/lock

alert_receiver=pengdan@oneboxtech.com

binary=bin/crawler
params="--log_dir=log --config_path=conf/crawler.cfg --http_port_for_counters=12000 \
--redis_servers_list=data/redis_url_servers.txt --job_retry_times=5"

status_file=status/crawler

time_to_restart=300
interval_to_report=1800
