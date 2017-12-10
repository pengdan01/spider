#!/binbash
selector_input=""

## 检查 监控的 用户输入目录, 看是否有新的需要 处理的输入
user_input_data_is_ready=n
${HADOOP_HOME}/bin/hadoop fs -test -e "${user_input_monitor_dir}/${todo_filename}"
if [ $? -eq 0 ]; then
  user_input_data_is_ready=y
  rm -f ${status_dir}/todo.tmp
  ${HADOOP_HOME}/bin/hadoop fs -get ${user_input_monitor_dir}/${todo_filename} ${status_dir}/todo.tmp
  if [ $? -ne 0 ]; then
    err_msg="Fail: hadoop fs -get ${user_input_monitor_dir}/${todo_filename} ${status_dir}/todo.tmp"
    echo ${err_msg}
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  while read line
  do
    [ ! "${line}" ] && continue
    if [ "${selector_input}" ]; then
      selector_input=${selector_input},${line}
    else
      selector_input=${selector_input},${line}
    fi
  done < ${status_dir}/todo.tmp
fi

## 从 newlink donelist 获取爬虫提取出来的新发现的链接
## newlink_donelist format like: newlink_dir \t timestamp
newlink_data_is_ready=n
local_newlink_donelist=${status_dir}/newlink_donelist
local_newlink_donelist_tmp=${status_dir}/newlink_donelist.tmp
link_input=""
${HADOOP_HOME}/bin/hadoop fs -test -e ${newlink_donelist}
if [ $? -ne 0 ]; then
  echo "Warning: newlink donelist[${newlink_donelist}] not exist"
else
  rm -f ${local_newlink_donelist_tmp} 
  ${HADOOP_HOME}/bin/hadoop fs -get ${newlink_donelist} ${local_newlink_donelist_tmp}
  if [ $? -ne 0 ]; then
    err_msg="FAIL: hadoop fs -get ${newlink_donelist} ${local_newlink_donelist_tmp}"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  if [ ! -s ${local_newlink_donelist_tmp} ]; then
    echo "Warning: newlink donelist[${newlink_donelist}] is empty."
  else
    newlink_data_is_ready=y
    last_handled_dir=""
    finish_line=0
    if [ -e ${local_newlink_donelist} ]; then
      diff ${local_newlink_donelist} ${local_newlink_donelist_tmp} 
      if [ $? -eq 0 ]; then 
        newlink_data_is_ready=n && echo "Warning: newlink donelist is the same as the last one"
      else
        last_handled_dir=`cat ${local_newlink_donelist} | tail -1 | awk '{print $1}'`
        finish_line=`grep -n "${last_handled_dir}" ${local_newlink_donelist_tmp} | awk -F':' '{print $1}'`
        [ ! "${finish_line}" ] && finish_line=0
      fi
    fi
    ## get the newlink hadoop dir
    if [ ${newlink_data_is_ready} = "y" ]; then
      cnt=0
      while read line
      do
        ((++cnt))
        tmpdir=`echo $line | awk '{print $1}'`
        [ $cnt -le $finish_line -o ! "${tmpdir}" ] && continue
        if [ "${link_input}" ]; then
          link_input=${link_input},${tmpdir}
        else
          link_input=${tmp_dir}
        fi
      done < ${local_newlink_donelist_tmp}
    fi
  fi
fi
if [ ! "${selector_input}" ]; then
  selector_input=${link_input}
elif [ "${link_input}" ]; then
  selector_input="${selector_input},${link_input}"
fi
## End of get newlink donelist

## Get the search log data
search_log_is_ready=n
local_search_log_donelist=${status_dir}/search_log_donelist
local_search_log_donelist_tmp=${status_dir}/search_log_donelist.tmp
if [ "${switch_use_search_log}" = "y" -o "${switch_use_search_log}" = "Y" ]; then
  $HADOOP_HOME/bin/hadoop fs -test -d ${search_log_dir}
  if [ $? -ne 0 ]; then
    err_msg="Error: search_log_dir[${search_log_dir}] not exsist"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -ls ${search_log_dir} | grep -v 'items' | awk '{print $NF}' > ${local_search_log_donelist_tmp} 
  if [ $? -ne 0 ]; then
    err_msg="FAIL: hadoop fs -ls ${search_log_dir}"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  latest_dir=`tail -1 ${local_search_log_donelist_tmp}` 
  last_handled_latest_dir=""
  finish_line=0
  search_log_is_ready=y
  if [ -e ${local_search_log_donelist} ]; then
    last_handled_latest_dir=`tail -1 ${local_search_log_donelist}`
    if [ "${last_handled_latest_dir}" = "${latest_dir}" ]; then
      echo "Warning, search log donelist no update"
      search_log_is_ready=n
    else
      finish_line=`grep -n "${last_handled_latest_dir}" ${local_search_log_donelist_tmp} | awk -F':' '{print $1}'`
      [ ! "${finish_line}" ] && finish_line=0
    fi
  fi
  if [ ${search_log_is_ready} = "y" ]; then
    cnt=1
    search_log=""
    while read line
    do
      if [ "${line}" -a ${cnt} -gt ${finish_line} ]; then
        tmp_input=`echo ${line} | awk '{print $1}'`
        if [ ! "${search_log}" ]; then
          search_log=${tmp_input}
        else
          search_log="${search_log},${tmp_input}"
        fi
        break  ## 正常情况下, 一天处理一个目录
      fi
      ((cnt++))
    done < ${local_search_log_donelist_tmp} 
    if [ "${search_log}" ]; then
      if [ "${selector_input}" ]; then
        selector_input="${selector_input},${search_log}"
      else
        selector_input="${search_log}"
      fi
    fi
  fi
fi
## End of get search log 

## Begin get pv log
pv_log_is_ready=n
local_pv_log_donelist=${status_dir}/pv_log_donelist
local_pv_log_donelist_tmp=${status_dir}/pv_log_donelist.tmp
if [ "${switch_use_pv_log}" = "y" -o "${switch_use_pv_log}" = "Y" ]; then
  $HADOOP_HOME/bin/hadoop fs -test -d ${pv_log_dir}
  if [ $? -ne 0 ]; then
    err_msg="Error: pv_log_dir[${pv_log_dir}] not exsist"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -ls ${pv_log_dir} | grep -v 'items' | awk '{print $NF}' > ${local_pv_log_donelist_tmp} 
  if [ $? -ne 0 ]; then
    err_msg="FAIL: hadoop fs -ls pv_log_dir[${pv_log_dir}] fail"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  latest_dir=`tail -1 ${local_pv_log_donelist_tmp}` 
  last_handled_latest_dir=""
  finish_line=0
  pv_log_is_ready=y
  if [ -e ${local_pv_log_donelist} ]; then
    last_handled_latest_dir=`tail -1 ${local_pv_log_donelist}`
    if [ "${last_handled_latest_dir}" = "${latest_dir}" ]; then
      echo "Warning, pv log donelist no update"
      pv_log_is_ready=n
    else
      finish_line=`grep -n "${last_handled_latest_dir}" ${local_pv_log_donelist_tmp} | awk -F':' '{print $1}'`
      [ ! "${finish_line}" ] && finish_line=0
    fi
  fi
  if [ ${pv_log_is_ready} = "y" ]; then
    cnt=1
    pv_log=""
    while read line
    do
      if [ "${line}" -a ${cnt} -gt ${finish_line} ]; then
        tmp_input=`echo ${line} | awk '{print $1}'`
        if [ ! "${pv_log}" ]; then
          pv_log=${tmp_input}
        else
          pv_log="${pv_log},${tmp_input}"
        fi
        break  ## 正常情况下, 一天处理一个目录
      fi
      ((cnt++))
    done < ${local_pv_log_donelist_tmp}  
    if [ "${pv_log}" ]; then
      if [ "${selector_input}" ]; then
        selector_input="${selector_input},${pv_log}"
      else
        selector_input="${pv_log}"
      fi
    fi
  fi
fi
## End of get pv log 
## Get the directory of navi boost data
navi_boost_is_ready=n
local_navi_log_donelist=${status_dir}/navi_log_donelist
local_navi_log_donelist_tmp=${status_dir}/navi_log_donelist.tmp
if [ "${switch_use_navi_boost}" = "y" -o "${switch_use_navi_boost}" = "Y" ]; then
  $HADOOP_HOME/bin/hadoop fs -test -d ${navi_boost_dir}
  if [ $? -ne 0 ]; then
    err_msg="Error: navi_log_dir[${navi_boost_dir}] not exsist" 
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -ls ${navi_boost_dir} | grep -v 'items' | awk '{print $NF}' > ${local_navi_log_donelist_tmp} 
  if [ $? -ne 0 ]; then
    err_msg="FAIL: hadoop fs -ls pv_log_dir[${navi_boost_dir}] fail"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  latest_dir=`tail -1 ${local_navi_log_donelist_tmp}` 
  last_handled_latest_dir=""
  finish_line=0
  navi_boost_is_ready=y
  if [ -e ${local_navi_log_donelist} ]; then
    last_handled_latest_dir=`tail -1 ${local_navi_log_donelist}`
    if [ "${last_handled_latest_dir}" = "${latest_dir}" ]; then
      echo "Warning, navi log donelist no update"
      navi_boost_is_ready=n
    else
      finish_line=`grep -n "${last_handled_latest_dir}" ${local_navi_log_donelist_tmp} | awk -F':' '{print $1}'`
      [ ! "${finish_line}" ] && finish_line=0
    fi
  fi
  if [ ${navi_boost_is_ready} = "y" ]; then
    cnt=1
    navi_log=""
    while read line
    do
      if [ "${line}" -a ${cnt} -gt ${finish_line} ]; then
        tmp_input=`echo ${line} | awk '{print $1}'`
        if [ ! "${navi_log}" ]; then
          navi_log=${tmp_input}
        else
          navi_log="${navi_log},${tmp_input}"
        fi
        break  ## 正常情况下, 一天处理一个目录
      fi
      ((cnt++))
    done < ${local_navi_log_donelist_tmp}  
    if [ "${navi_log}" ]; then
      if [ "${selector_input}" ]; then
        selector_input="${selector_input},${navi_log}"
      else
        selector_input="${navi_log}"
      fi
    fi
  fi
fi
## end navi boost
if [ ! "${selector_input}" ]; then
  warning_msg="Warning: input for selector is empty" 
  echo ${err_msg} 
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  ## 删除锁文件
  ${HADOOP_HOME}/bin/hadoop fs -rm ${lock_hdfs_file}
  exit 0
fi

## 需要根据 not_crawl_already_in_linkbase 来判断是否需要将 linkbase/linbkase_vip
## 作为输入. 如果需要作去重处理, 即: 如果待抓取网页已经在 linkbase 中存在则不进行抓取,
## 那么需要 添加最新 的linkbase 作为输入; 反之, 则不需要.
if [ ${not_crawl_already_in_linkbase} = 'y' -o ${not_crawl_already_in_linkbase} = 'Y']; then
  latest_link_base_dir=""
  if [ ! ${link_merge_donelist} ]; then
    echo "Waring, parameter link_merge_donelist is empty."
  else
    ${HADOOP_HOME}/bin/hadoop fs -test -e ${link_merge_donelist}
    if [ $? -ne 0 ]; then
      echo "Warning, link_merge_donelist is not exist"  
    else 
      rm -rf ${status_dir}/link_merge_donelist.tmp
      ${HADOOP_HOME}/bin/hadoop fs -get ${link_merge_donelist} ${status_dir}/link_merge_donelist.tmp
      if [ $? -ne 0 ]; then
        err_msg="FAIL: hadoop fs -get ${link_merge_donelist} ${status_dir}/link_merge_donelist.tmp" 
        echo ${err_msg} 
        SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
        exit 1
      fi
      latest_link_base_dir=`tail -1 ${status_dir}/link_merge_donelist.tmp | awk -F'\t' '{print $1}'` 
      if [ -s ${status_dir}/link_merge_donelist ]; then
        diff ${status_dir}/link_merge_donelist.tmp ${status_dir}/link_merge_donelist
        [ $? -eq 0 ] && echo "Warning, linkbase not update, donelist is the same as the last one handled"
      fi
    fi
  fi
  if [ "${latest_link_base_dir}" ]; then
    selector_input="${selector_input},${latest_link_base_dir}"
  fi
  ## Get VIP linkbase dir
  latest_link_base_dir_vip=""
  if [ ! ${link_merge_donelist_vip} ]; then
    echo "Waring, parameter link_merge_donelist_vip is empty."
  else
    ${HADOOP_HOME}/bin/hadoop fs -test -e ${link_merge_donelist_vip}
    if [ $? -ne 0 ]; then
      echo "Warning, link_merge_donelist_vip is not exist"  
    else 
      rm -rf ${status_dir}/link_merge_donelist_vip.tmp
      ${HADOOP_HOME}/bin/hadoop fs -get ${link_merge_donelist_vip} ${status_dir}/link_merge_donelist_vip.tmp
      if [ $? -ne 0 ]; then
        err_msg="FAIL: hadoop fs -get ${link_merge_donelist_vip} ${status_dir}/link_merge_donelist_vip.tmp" 
        echo ${err_msg} 
        SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
        exit 1
      fi
      latest_link_base_dir_vip=`tail -1 ${status_dir}/link_merge_donelist_vip.tmp | awk -F'\t' '{print $1}'` 
      if [ -s ${status_dir}/link_merge_donelist_vip ]; then
        diff ${status_dir}/link_merge_donelist_vip.tmp ${status_dir}/link_merge_donelist_vip
        [ $? -eq 0 ] && echo "Warning, linkbase not update, donelist is the same as the last one handled"
      fi
    fi
  fi
  if [ "${latest_link_base_dir_vip}" ]; then
    selector_input="${selector_input},${latest_link_base_dir_vip}"
  fi
fi
## End of get the directory of latest linkbase

## Check Index Mode exist or not
cache_archive_files=""
if [ ${switch_use_index_model} = "y" -o ${switch_use_index_model} = "Y" ]; then
  $HADOOP_HOME/bin/hadoop fs -test -e ${index_model_hdfs_path} 
  if [ $? -ne 0 ]; then
    err_msg="Error, Index model[${index_model_hdfs_path}] not exist"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  if [ ! ${index_model_name} ]; then
    err_msg="Error, parameter index_model_name is empty"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  $HADOOP_HOME/bin/hadoop fs -test -e ${index_model_data_dict} 
  if [ $? -ne 0 ]; then
    err_msg="Error, Index model data dict[${index_model_data_dict}] not exist"
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  cache_archive_files="--mr_cache_files ${index_model_hdfs_path}#${index_model_name} \
  --mr_cache_archives ${index_model_data_dict}#data,/app/crawler/app_data/web.tar.gz#web"
else
  cache_archive_files="--mr_cache_archives /app/crawler/app_data/web.tar.gz#web"
fi

## Start binary program 
Round1_output="${selector_outdir}/${timestamp}/NotSorted.st"
Round1_output_tmp="${selector_outdir}/${timestamp}/_tmp_NotSorted.st"
mapper=".build/opt/targets/crawler/selector/mr_selector_r1_mapper \
--switch_use_index_model=${switch_use_index_model} --index_model_name=${index_model_name} \
--use_mmp_mode=true --seed_url_type=1 --switch_filter_rules=true --logtostderr"
reducer=".build/opt/targets/crawler/selector/mr_selector_r1_reducer \
--switch_use_index_model=${switch_use_index_model} --index_model_name=${index_model_name} \
--model_score_threshold=${model_score_threshold} --use_mmp_mode=true \
--not_crawle_already_in_linkbase=${not_crawl_already_in_linkbase} \
--logtostderr"
build_tools/mapreduce/submit.py \
  --mr_ignore_input_error \
  --mr_slow_start_ratio 0.95 \
  ${cache_archive_files} \
  --mr_map_cmd ${mapper} --mr_reduce_cmd ${reducer} \
  --mr_input ${selector_input} --mr_input_format streaming_text \
  --mr_output ${Round1_output_tmp} --mr_output_format streaming_text \
  --mr_min_split_size 1024 --mr_reduce_capacity 1000 --mr_map_capacity 1000 \
  --mr_reduce_tasks ${reduce_task_num} \
  --mr_job_name "wly_selector_round1"
if [ $? -ne 0 ]; then
  err_msg="hadoop job fail: wly-selector-round1"
  echo ${err_msg} 
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi
$HADOOP_HOME/bin/hadoop fs -mv ${Round1_output_tmp} ${Round1_output}
if [ $? -ne 0 ]; then
  err_msg="FAIL: hadoop fs -mv ${Round1_output_tmp} ${Round1_output}"
  echo ${err_msg} 
  SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
  exit 1
fi

## 已经成功处理 (selector 阶段) 用户输入, 将 todo 重新改名成 done
if [ ${user_input_data_is_ready} = "y" ]; then
  ${HADOOP_HOME}/bin/hadoop fs -test -e ${user_input_monitor_dir}/${done_filename}
  if [ $? -eq 0 ]; then
    ${HADOOP_HOME}/bin/hadoop fs -mv ${user_input_monitor_dir}/${done_filename} ${user_input_monitor_dir}/${done_filename}.${timestamp}
    err_msg="FAIL: hadoop fs -mv ${user_input_monitor_dir}/${done_filename} ${user_input_monitor_dir}/${done_filename}.${timestamp}" 
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
  ${HADOOP_HOME}/bin/hadoop fs -mv ${user_input_monitor_dir}/${todo_filename} ${user_input_monitor_dir}/${done_filename}
  if [ $? -ne 0 ]; then
    err_msg="FAIL: hadoop fs -mv ${user_input_monitor_dir}/${todo_filename} ${user_input_monitor_dir}/${done_filename}" 
    echo ${err_msg} 
    SendAlertViaMail "${err_msg}" "${alert_mail_receiver}"
    exit 1
  fi
fi

if [ ${newlink_data_is_ready} = "y" ]; then
  mv ${local_newlink_donelist_tmp} ${local_newlink_donelist}
fi
if [ ${search_log_is_ready} = "y" ]; then 
  mv ${local_search_log_donelist_tmp} ${local_search_log_donelist}
fi
if [ ${navi_boost_is_ready} = "y" ]; then 
  mv ${local_navi_log_donelist_tmp} ${local_navi_log_donelist}
fi
if [ ${pv_log_is_ready} = "y" ]; then 
  mv ${local_pv_log_donelist_tmp} ${local_pv_log_donelist}
fi
if [ -e ${status_dir}/link_merge_donelist.tmp ]; then
  mv ${status_dir}/link_merge_donelist.tmp ${status_dir}/link_merge_donelist
fi
if [ -e ${status_dir}/link_merge_donelist_vip.tmp ]; then
  mv ${status_dir}/link_merge_donelist_vip.tmp ${status_dir}/link_merge_donelist_vip
fi
