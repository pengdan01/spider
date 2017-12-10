#!/bin/bash
set -u
.bin/task_pusher --datapath fail --ip 180.153.240.26 --port 20080 --log_dir log --job_default_priority=0
