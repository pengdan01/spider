#!/bin/bash

pidfile=run/pid
lockfile=run/lock

alert_receiver=pengdan@oneboxtech.com

binary=bin/scheduler
params="--log_dir=log --in_queue_ip 127.0.0.1 --in_queue_port 10020  --out_queue_ip 127.0.0.1 \
--out_queue_port 10080 --http_port_for_counters=14000 --max_buffer_size=100000"

status_file=status/crawler

time_to_restart=300
interval_to_report=1800
