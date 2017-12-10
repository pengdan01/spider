#!/bin/bash
set -u
.bin/task_pusher --datapath urls_to_push.txt --ip 10.115.98.106 --port 20080 --log_dir log
