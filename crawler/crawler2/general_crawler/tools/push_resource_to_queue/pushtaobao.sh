# 将上传到 hadoop 上的 HbaseDump 格式(sf) resource 灌到 Job queue 中
# step1. 下载 HbaseDump 到本地
# step2. 执行本脚本, 进行灌 Job queue 
for i in '`ls todo`'
do
  /home/crawler/build_tools/mapreduce/cat_sfile.sh todo/$i | ./pushtaobao --mr_method=map --crawler_queue_ip=180.153.240.33 --crawler_queue_port=18888
  [ $? -ne 0 ] && exit 1
done
exit 0
