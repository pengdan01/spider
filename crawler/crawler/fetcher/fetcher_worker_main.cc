#include <string>
#include <iostream>
#include <fstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/closure.h"
#include "base/time/timestamp.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_printf.h"
#include "base/thread/thread_pool.h"
#include "base/common/counters.h"
#include "base/file/file_util.h"
// #include "rpc/utils/expose_counters.h"

#include "crawler/util/command.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/fetcher/multi_fetcher.h"

// 对于一站点, 最多有 3 个 fetcher(一般一台及其部署一个 fetcher) 同时访问
DEFINE_int32(max_concurrent_access_fetcher, 3, "对于一站点, 最多有 3 个 fetcher(一台机器部署一个 fetcher) 同时访问");  // NOLINT

DEFINE_string(hdfs_working_dir, "/app/crawler/general_crawler", "the hdfs working directory for crawler");
DEFINE_string(dns_servers, "", "DNS server list.");
DEFINE_int32(thread_num, 8, "thread# in thread_pool");
DEFINE_string(ip_range_data_file, "", "the file contains ip range assigned to China");
DEFINE_int32(crawler_worker_id, -1, "the id of current crawler worker");
DEFINE_string(working_dir, "./tmp/saver", "the local working directory for crawler saver");
const int64 kBinaryFileMaxSize = 268435456;  // 256 MB
const int64 kTextFileMaxSize = 268435456;  // 256 MB
DEFINE_int64(binary_file_max_size, kBinaryFileMaxSize, "the max size of local temporary binary file");
DEFINE_int64(text_file_max_size, kTextFileMaxSize, "the max size of local temporary text file");

DEFINE_string(status_dir, "~/crawler_status/fetcher", "存放抓取中间状态");
int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "fetcher worker main");
  // rpc_utils::ExposeCounters();

  LOG_IF(FATAL, FLAGS_thread_num <= 0 || FLAGS_thread_num > 32) << "thread_num should in (0,32].";
  LOG_IF(FATAL, FLAGS_crawler_worker_id < 0) << "crawler_worker_id is should start from 0";
  LOG_IF(FATAL, FLAGS_working_dir.empty()) << "local working_dir is empty";
  LOG_IF(FATAL, FLAGS_hdfs_working_dir.empty()) << "hdfs_working_dir is empty";
  LOG_IF(FATAL, FLAGS_max_concurrent_access_fetcher <= 0) << "max_concurrent_access_fetcher max > 0";

  LOG_IF(FATAL, FLAGS_binary_file_max_size <= 0) << "Binary_file_max_size: " << FLAGS_binary_file_max_size;
  LOG_IF(FATAL, FLAGS_text_file_max_size <= 0) << "Text_file_max_size:" << FLAGS_text_file_max_size;

  // Build IP Scope Dict
  LOG_IF(FATAL, FLAGS_ip_range_data_file.empty()) << "ip_range_data_file is empty";
  std::vector<crawler::EdgeNode> ip_scopes;
  bool ret = crawler::BuildIPScopeFromFile(FLAGS_ip_range_data_file.c_str(), &ip_scopes);
  LOG_IF(FATAL, !ret) << "crawler::BuildIPScopeFromFile() fail.";
  LOG(INFO) << "ip_scopes size = " << ip_scopes.size();

  std::string page_output_dir, link_output_dir, css_output_dir, image_output_dir, newlink_output_dir;


  bool status;
  int priority;
  std::string cmd;
  std::string hdfs_url_file, host_load_file;  // input
  std::string hdfs_output_root_dir, hdfs_status_output_dir;
  std::string local_output_donelist, output_donelist;
  std::string input_donelist_record;  // 存放当前正在处理的那条 input donelist 记录
  std::string format_worker_id = base::StringPrintf("%03d", FLAGS_crawler_worker_id);
  while (true) {
    // 检测 是否有新的任务列表
    status = crawler::CheckNewTask(FLAGS_hdfs_working_dir, format_worker_id, &hdfs_url_file,
                                   &host_load_file, &priority, &hdfs_output_root_dir, &hdfs_status_output_dir,
                                   &local_output_donelist, &output_donelist, &input_donelist_record);
    if (status == false) {  // 没有更新的 task; 睡 30 秒 后再查看
      LOG(ERROR) << "I am hungry, give me some tasks to eat...";
      sleep(30);
      continue;
    }
    // 测试并下载文件到 hdfs_url_file 和站点压力控制文件 host_load_file 到本地
    std::string local_tmp_url_file = "./tmp_url_file";
    base::FilePath file_path_tmp_url(local_tmp_url_file);
    LOG_IF(ERROR, !base::file_util::Delete(file_path_tmp_url, true)) << "Delete local file fail, file: "
                                                                     << local_tmp_url_file;
    // 下载 hdfs url file 文件到本地
    cmd = "${HADOOP_HOME}/bin/hadoop fs -get " + hdfs_url_file + " " + local_tmp_url_file;
    if (!crawler::execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Error, execute cmd fail, cmd: " << cmd;
      continue;
    }
    std::string local_tmp_host_load = "./tmp_host_load";
    base::FilePath file_path_tmp_host_load(local_tmp_host_load);
    LOG_IF(ERROR, !base::file_util::Delete(file_path_tmp_host_load, true)) << "Delete local file fail, file: "
                                                                           << local_tmp_host_load;
    // 下载 hdfs host load 文件到本地
    cmd = "${HADOOP_HOME}/bin/hadoop fs -get " + host_load_file + " " + local_tmp_host_load;
    if (!crawler::execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Error, execute cmd fail, cmd: " << cmd;
      continue;
    }
    // 解析 task 文件
    std::list<crawler::UrlHostIP> task_item_list;
    if (!crawler::ParseNewTaskFile(local_tmp_url_file, &task_item_list)) {
      LOG(ERROR) << "ParseNewTaskFile() fail, task file: " << local_tmp_url_file;
      continue;
    }
    // 任务切分
    std::vector<std::list<crawler::UrlHostIP>> each_thread_tasks(FLAGS_thread_num);
    if (!crawler::SplitTaskUrl(task_item_list, FLAGS_thread_num, FLAGS_max_concurrent_access_fetcher,
                               &each_thread_tasks)) {
      LOG(ERROR) << "Fail in SplitTaskUrl()";
      continue;
    }
    // 计算并创建任务的 hdfs 输出路径:
    if (!crawler::BuildHdfsOutputDir(hdfs_output_root_dir, &page_output_dir, &link_output_dir,
                                     &css_output_dir, &image_output_dir, &newlink_output_dir)) {
      LOG(ERROR) << "Fail in BuildHdfsOutputDir()";
      continue;
    }
    thread::ThreadPool my_thread_pool(FLAGS_thread_num);
    for (int i = 0; i < FLAGS_thread_num; ++i) {
      if (each_thread_tasks[i].size() == 0) {  // 对应线程没有分到 下载任务, 直接跳过
        LOG(ERROR) << "Info, thread not assigned task, thread id: " << i;
        continue;
      }
      crawler::TaskStatistics *stat = new crawler::TaskStatistics();
      CHECK_NOTNULL(stat);
      crawler::ResourceSaver *saver =
          new crawler::ResourceSaver(page_output_dir, link_output_dir, css_output_dir, image_output_dir,
                                     newlink_output_dir, hdfs_status_output_dir, FLAGS_working_dir,
                                     FLAGS_text_file_max_size, FLAGS_binary_file_max_size);
      CHECK_NOTNULL(saver);
      // XXX(pengdan): stat 和 saver 由每个线程在负责释放(delete)
      crawler::ThreadParameter parameter("", stat, saver, format_worker_id,
                                         // The only way to make the compiler happy.
                                         (const std::vector<crawler::EdgeNode> *)&ip_scopes,
                                         FLAGS_dns_servers,
                                         &each_thread_tasks[i],
                                         local_tmp_host_load, FLAGS_max_concurrent_access_fetcher);
      my_thread_pool.AddTask(NewCallback(crawler::CrawlerWorkerThreadNew, parameter));
    }
    my_thread_pool.JoinAll();
    std::string record = base::StringPrintf("%s\t%s\t%s\n", hdfs_output_root_dir.c_str(),
                                            hdfs_status_output_dir.c_str(), input_donelist_record.c_str());
    LOG_IF(ERROR, !crawler::AppendOutputDonelist(local_output_donelist, output_donelist, record))
        << "Failed in append output donelsit, input url: " << hdfs_url_file << ", host load file: "
        << host_load_file << ", output root dir: " << hdfs_output_root_dir;
    // 输出 counter 的信息到 LOG(INFO)
    // base::LogAllCounters();
  }
  return 0;
}
