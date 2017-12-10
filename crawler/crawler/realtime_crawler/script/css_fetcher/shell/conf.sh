#!/bin/bash

pidfile=run/pid
lockfile=run/lock

alert_receiver=pengdan@oneboxtech.com

css_cache_server_list_file=data/cache_server_list.txt
ip_load_file=data/ip_load

page_analyzer_queue_ip=180.153.240.26
page_analyzer_queue_port=20081
in_queue_ip=180.153.240.26
in_queue_port=20071

binary=.bin/css_fetcher
params="--log_dir=log --css_cache_server_list_file=${css_cache_server_list_file} \
--page_analyzer_queue_ip=${page_analyzer_queue_ip} \
--page_analyzer_queue_port=${page_analyzer_queue_port} \
--in_queue_ip=${in_queue_ip} --in_queue_port=${in_queue_port} \ 
--ip_load_file=${ip_load_file} --http_port_for_counters=39000 \
--max_css_cache_size=3000000"

interval_to_report=5

time_to_restart=2
