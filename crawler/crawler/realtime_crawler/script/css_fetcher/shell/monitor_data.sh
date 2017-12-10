#!/bin/bash

# 此脚本负责监控 以 prefix.timestamp.data 形式的文件
# 如果此文件出现了对应的 done 文件 (prefix.timetamp.done)
# 那么文件将被上传到指定 hadoop 目录
#!bin/bash

# 此脚本负责监控 以 prefix.timestamp.data 形式的文件
# 如果此文件出现了对应的 done 文件 (prefix.timetamp.done)
# 那么文件将被上传到指定 hadoop 目录

source shell/conf.sh

monitor_dir=$1
monitor_file_prefix=$2
hadoop_dir=$3

while true
do
    datalist=`ls -l ${monitor_dir} | grep -o -E "${monitor_file_prefix}\.[0-9]{14}" | sort | uniq`;
    for line in $datalist; do
        if [ -e "${monitor_dir}/${line}.done" ]; then
            # 从文件当中提取日期
            date=`echo ${line} | grep -o -E [0-9]{8} `;
            output_dir=${hadoop_dir}/${date}/
            $HADOOP_HOME/bin/hadoop fs -mkdir ${output_dir} &> /dev/null
            $HADOOP_HOME/bin/hadoop fs -put "${monitor_dir}/${line}.data" /tmp/${line}.data
            if [ $? -ne 0 ]; then
                echo "Failed to upload";
                continue;
            fi

	    # 上传成功后移动至路径
	    $HADOOP_HOME/bin/hadoop fs -mv /tmp/${line}.data ${output_dir};
            rm ${monitor_dir}/${line}.data;
            rm ${monitor_dir}/${line}.done;
            echo "success update data: ${monitor_dir}/${line}.data";
        fi
    done

    sleep 100;
done
