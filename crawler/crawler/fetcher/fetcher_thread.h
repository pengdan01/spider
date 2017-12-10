#pragma once

#include <utility>
#include <string>
#include <vector>
#include <list>

#include "crawler/util/ip_scope.h"
#include "crawler/util/dns_resolve.h"
#include "crawler/fetcher/crawler_analyzer.h"
#include "crawler/fetcher/resource_saver.h"

namespace crawler {

struct TaskParameter {
  const std::string hadoop_file_path;
  TaskParameter() {}
  explicit TaskParameter(const std::string &hdfs_path): hadoop_file_path(hdfs_path) {}
};

struct TaskStatistics {
  int64 link_total_num;
  int64 link_failed_get_domain_num;
  int64 link_failed_get_ip_num;
  int64 link_foreign_num;
  int64 link_fetch_timeout_num;
  int64 link_failed_get_page_num;
  int64 link_fetch_ok_num;
  int64 css_fetch_num;
  int64 image_fetch_num;
  TaskStatistics():link_total_num(0), link_failed_get_domain_num(0), link_failed_get_ip_num(0),
    link_foreign_num(0), link_fetch_timeout_num(0), link_failed_get_page_num(0), link_fetch_ok_num(0),
    css_fetch_num(0), image_fetch_num(0) {}
};

struct ThreadParameter {
  TaskParameter task;
  TaskStatistics *statistics;
  ResourceSaver *saver;
  std::string worker_id;
  const std::vector<EdgeNode> *ip_scopes;
  std::string dns_servers;
  std::list<crawler::UrlHostIP> *url_list;
  std::string hostload_conf_file;
  int max_concurrent_access_fetcher;
  ThreadParameter(const std::string &hdfs_path, TaskStatistics *stats,
                  ResourceSaver *saver, const std::string worker_id, const std::vector<EdgeNode> *ip_scopes,
                  const std::string &dns_server,
                  const std::string &hostload_conf, int access_fetcher):
      task(hdfs_path), statistics(stats), saver(saver), worker_id(worker_id),
      ip_scopes(ip_scopes), dns_servers(dns_server),
      url_list(NULL), hostload_conf_file(hostload_conf), max_concurrent_access_fetcher(access_fetcher) {}
  ThreadParameter(const std::string &hdfs_path, TaskStatistics *stats,
                  ResourceSaver *saver, const std::string worker_id, const std::vector<EdgeNode> *ip_scopes,
                  const std::string &dns_server,
                  std::list<crawler::UrlHostIP> *url_list, const std::string &hostload_conf,
                  int access_fetcher):
      task(hdfs_path), statistics(stats), saver(saver), worker_id(worker_id),
      ip_scopes(ip_scopes), dns_servers(dns_server),
      url_list(url_list), hostload_conf_file(hostload_conf), max_concurrent_access_fetcher(access_fetcher) {}
};

// void CrawlerWorkerThread(ThreadParameter parameter);
void CrawlerWorkerThreadNew(ThreadParameter parameter);
// |hdfs_file| 记录了爬虫输出路径
bool BuildHdfsOutputDir(const std::string &hdfs_output_root_dir, std::string *page_dir, std::string *link_dir,
                  std::string *css_dir, std::string *image_dir, std::string *newlink_dir);
bool CheckNewTask(const std::string &target_dir, const std::string &worker_id,
                  std::string *url, std::string *host_load, int *priority, std::string *hdfs_output_dir,
                  std::string *hdfs_status_output_dir, std::string *local_output_donelist,
                  std::string *hdfs_output_donelist, std::string *input_donelist_record);
bool ParseNewTaskFile(const std::string &local_tmp_url_file, std::list<UrlHostIP> *urls);
bool SplitTaskUrl(const std::list<crawler::UrlHostIP> &task_item_list, int FLAGS_thread_num,
                  int FLAGS_max_concurrent_access_fetche,
                  std::vector<std::list<crawler::UrlHostIP>> *each_thread_tasks);
bool AppendOutputDonelist(const std::string &local_output_donelist, const std::string &outputdonelist,
                          const std::string &record);
bool ParseTaskListFile(const std::string &total_task_filename, std::vector<std::string> &hdfs_files_vector);

}  // end namespace
