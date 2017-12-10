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
    datalist=`ls -l ${monitor_dir}/${monitor_file_prefix}.*.done 2>/dev/null | sort | uniq`;
    for line in $datalist; do
        if [ -e "${line}" ]; then
            # 从文件当中提取日期
            date=`echo ${line} | grep -o -E [0-9]{8} `;
            output_dir=${hadoop_dir}/${date}.st/
            $HADOOP_HOME/bin/hadoop fs -mkdir ${output_dir} &> /dev/null
            datafile=`echo ${line} | awk '{gsub("done", "data"); print $0}'`
            filename=`echo $datafile | awk -F'\t' '{print $NF}'`
            $HADOOP_HOME/bin/hadoop fs -rm /tmp/$filename &>/dev/null
            $HADOOP_HOME/bin/hadoop fs -put "$datafile" /tmp/$filename
            if [ $? -ne 0 ]; then
                echo "Failed to upload";
                rm -f $line
                continue;
            fi

	          # 上传成功后移动至路径
	          $HADOOP_HOME/bin/hadoop fs -mv /tmp/${filename} ${output_dir}
            rm -f $line 
            rm -f $datafile 
            echo "success update data: $datafile"
        fi
    done
    sleep 100;
done
