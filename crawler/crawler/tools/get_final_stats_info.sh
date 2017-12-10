#!/bin/bash
pssh_dir=wly_crawler_online/build_tools/pssh

cmd="tail -3  ~/crawler_log/crawler_fetcher.INFO | head -1"

${pssh_dir}/pssh -i -h hosts_info/crawler_host.txt -O StrictHostKeyChecking=no "${cmd}" > stats_info.tmp

grep "Finished tasks number" stats_info.tmp > stats_info 

rm -f stats_info.tmp

total_link_to_fetch=0
total_html_fetched=0
total_css_fetched=0
total_image_fetched=0
average_QPS=0

cnt=0
while read line
do
  link_to_fetch=`echo $line | awk -F',' '{print $4}'`
  link_to_fetch=`echo $link_to_fetch | awk -F'#' '{print $NF}'`
  ((total_link_to_fetch = total_link_to_fetch + link_to_fetch))

  html_fetched=`echo $line | awk -F',' '{print $5}'`
  html_fetched=`echo $html_fetched| awk -F'#' '{print $NF}'`
  ((total_html_fetched = total_html_fetched + html_fetched))

  css_fetched=`echo $line | awk -F',' '{print $6}'`
  css_fetched=`echo $css_fetched| awk -F'#' '{print $NF}'`
  ((total_css_fetched = total_css_fetched + css_fetched))

  image_fetched=`echo $line | awk -F',' '{print $7}'`
  image_fetched=`echo $image_fetched| awk -F'#' '{print $NF}'`
  ((total_image_fetched = total_image_fetched + image_fetched))

  qps=`echo $line | awk -F',' '{print $NF}'`
  qps=`echo $qps | awk -F'=' '{print $NF}'`
  average_QPS=`echo "$average_QPS + $qps" | bc`
  
  ((++cnt))
done < stats_info

average_QPS=`echo "$average_QPS / $cnt" | bc`

echo "total_link_to_fetch: $total_link_to_fetch"
echo "total_html_fetched:  $total_html_fetched"
echo "total_css_fetched:  $total_css_fetched"
echo "total_image_fetched:  $total_image_fetched"
echo "Average QPS:  $average_QPS"

exit 0
