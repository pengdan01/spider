#!/bin/bash

source shell/conf.sh

monitor_dir=$1
monitor_file_prefix=$2
hadoop_dir=$3

while true
do
    datalist=`ls -l ${monitor_dir}/${monitor_file_prefix}.*.done 2>/dev/null | sort | uniq`;
    for line in $datalist; do
        if [ -e "${line}" ]; then
            # 获取上传时的日期和小时: 历史没有上传的 网页也会上传的本目录 
            date=`date -d "+30 mins" +%Y%m%d`
            hour=`date -d "+30 mins" +%H`
            output_dir=${hadoop_dir}/${date}/${hour}/page.sf/
            $HADOOP_HOME/bin/hadoop fs -mkdir ${output_dir} &> /dev/null
            datafile=`echo ${line} | awk '{gsub("done", "data"); print $0}'`
            filename=`echo $datafile | awk -F'\t' '{print $NF}'`
            $HADOOP_HOME/bin/hadoop fs -rm /tmp/$filename &>/dev/null
            bash -x ~/build_tools/mapreduce/sfile_writer.sh -output /tmp/$filename < ${datafile}
            if [ $? -ne 0 ]; then
                echo "Failed to upload using sfile_writer.sh";
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
