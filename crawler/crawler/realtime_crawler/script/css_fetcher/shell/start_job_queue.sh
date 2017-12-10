## 10.115.86.99 20071
#!/bin/bash
[ $# -ne 1 ] && echo "Usage: $0 queue_port_num" && exit 1
port_num=$1
.bin/beanstalkd  -p $port_num -z 10240000 -s 30485760
