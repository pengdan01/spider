#!/bin/bash

pidfile=run/pid
lockfile=run/lock

alert_receiver=pengdan@oneboxtech.com

binary=.bin/realtime_crawler
params="--log_dir=log --config_path=conf/realtime.cfg --http_port_for_counters=34000"

time_to_restart=300
status_file=status/crawler

time_to_restart=300
interval_to_report=1800
