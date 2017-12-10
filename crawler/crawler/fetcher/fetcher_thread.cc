#include "crawler/fetcher/fetcher_thread.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <algorithm>

#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/hash_function/fast_hash.h"
#include "base/thread/thread_pool.h"
#include "base/time/timestamp.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/file/file_util.h"
#include "base/random/pseudo_random.h"
#include "web/url/gurl.h"
#include "crawler/util/command.h"
#include "crawler/fetcher/multi_fetcher.h"

namespace crawler {

static const int kPriorityMaxId = 5;  // 优先级输入 [1, 5], 最高优先级为 kPriorityMaxId

bool LoadDonelistToVector(const std::string &local_input_tmp_donelist,
                          std::vector<std::string> *donelist_vector) {
  CHECK_NOTNULL(donelist_vector);
  std::ifstream myfin(local_input_tmp_donelist);
  CHECK(myfin.good()) << "open file fail, file: " << local_input_tmp_donelist;
  std::string line;
  donelist_vector->clear();
  while (getline(myfin, line)) {
    if (line.empty() || line[0] == '#') continue;
    donelist_vector->push_back(line);
  }
  CHECK(myfin.eof());
  return true;
}

// output donelist 记录格式: output_dir \t status_data_dir \t input_donelist_record
bool LoadDonelistToSet(const std::string &local_output_tmp_donelist,
                       std::set<std::string> *donelist_set) {
  CHECK_NOTNULL(donelist_set);
  std::ifstream myfin(local_output_tmp_donelist);
  CHECK(myfin.good()) << "open file fail, file: " << local_output_tmp_donelist;
  std::vector<std::string> tokens;
  std::string line;
  donelist_set->clear();
  while (getline(myfin, line)) {
    if (line.empty() || line[0] == '#') continue;
    tokens.clear();
    int field_num = base::Tokenize(line, "\t", &tokens);
    LOG_IF(FATAL, field_num != 3) << "Record format error, line: " << line;
    donelist_set->insert(tokens[2]);
  }
  CHECK(myfin.eof());
  return true;
}

// 检察 是否由新的输入, 如果有, 设置输入的 url 文件以及 任务压力控制参数 以及优先级 id, 任务最终输出的路径
// 并返回 true
// 若没有新任务, 则 返回 false
bool CheckNewTask(const std::string &target_dir, const std::string &worker_id,
                  std::string *url_file,
                  std::string *host_load, int *priority, std::string *hdfs_output_dir,
                  std::string *hdfs_output_status_dir, std::string *local_output_donelist_ret,
                  std::string *output_donelist_ret, std::string *input_donelist_record) {
  std::string input_path_prefix = target_dir + "/fetcher_" + worker_id + "/input/p";
  std::string output_path_prefix = target_dir + "/fetcher_" + worker_id + "/output/p";
  std::string input_donelist, output_donelist;
  std::string cmd;
  std::vector<std::string> input_donelist_vector;
  std::set<std::string> output_donelist_set;
  for (int i = kPriorityMaxId; i > 0; --i) {
    input_donelist = base::StringPrintf("%s%d/donelist", input_path_prefix.c_str(), i);
    cmd = "${HADOOP_HOME}/bin/hadoop fs -test -e " + input_donelist;
    // 没有找到该优先级下的 input donelist, 继续找下一个优先级下的 donelist
    if (!execute_cmd_with_retry(cmd, 1)) {
      LOG(WARNING) << "Warning, input donelist not found, donelist is: " << input_donelist
                 << ", try to find input donelist of next priority";
      continue;
    } else {
      // 获取 input donelist 到本地, 并检查是否更新
      std::string local_input_tmp_donelist = base::StringPrintf("./input_donelist_%s_%d.tmp",
                                                                worker_id.c_str(), i);
      // 删除本地的临时文件
      base::FilePath file_path_tmp_input_donelist(local_input_tmp_donelist);
      LOG_IF(FATAL, !base::file_util::Delete(file_path_tmp_input_donelist, true))
          << "Delete local file fail, file: " << local_input_tmp_donelist;
      // 从 hadoop 上获取 input donelist 到本地
      cmd = "${HADOOP_HOME}/bin/hadoop fs -get " + input_donelist + " " + local_input_tmp_donelist;
      if (!execute_cmd_with_retry(cmd, 3)) {
        LOG(ERROR) << "Error, cmd fail, cmd is: " << cmd << ", try to find next priority donelist";
        continue;
      }
      // 加载 input_donelist 到 vector 中
      // 会清空 input_donelist_vector
      LOG_IF(FATAL, !LoadDonelistToVector(local_input_tmp_donelist, &input_donelist_vector))
          << "file: " << local_input_tmp_donelist;
      if (input_donelist_vector.empty()) {
        LOG(ERROR) << "input donelist file is empty, file: " << input_donelist
                   << " try to find next priority donelist";
        continue;
      }
      // 从 hadoop 上获取 output donelist 到本地 (爬虫每次处理完后, 会追加一条记录到 output donelist)
      std::string output_prefix = base::StringPrintf("%s%d", output_path_prefix.c_str(), i);
      output_donelist = output_prefix + "/donelist";
      std::string local_output_tmp_donelist = base::StringPrintf("./output_donelist_%s_%d.tmp",
                                                                 worker_id.c_str(), i);
      // 删除本地的临时文件 output donelist
      base::FilePath file_path_tmp_output_donelist(local_output_tmp_donelist);
      LOG_IF(WARNING, !base::file_util::Delete(file_path_tmp_output_donelist, true))
          << "Delete local file fail, file: " << local_output_tmp_donelist;
      cmd = "${HADOOP_HOME}/bin/hadoop fs -test -e " + output_donelist;
      if (!execute_cmd_with_retry(cmd, 1)) {  // output donelist 不存在
        // 直接获取 input donelist 最后一条记录, 前提: input donelist 按照时间搓 和 id 有序
        std::string input_file_prefix = input_donelist_vector[input_donelist_vector.size()-1];
        *url_file = input_file_prefix + ".url";
        *host_load = input_file_prefix + ".ipload";
        std::string::size_type pos = input_file_prefix.rfind("/");
        LOG_IF(FATAL, pos == std::string::npos) << "url_file name is not valid, filename: " << *url_file;
        *priority = i;
        *hdfs_output_dir = output_prefix + "/" + input_file_prefix.substr(pos+1) + ".output";
        *hdfs_output_status_dir = output_prefix + "/" + input_file_prefix.substr(pos+1) + ".status.st";
        *output_donelist_ret = output_donelist;
        *local_output_donelist_ret = local_output_tmp_donelist;
        *input_donelist_record = input_file_prefix;
        return true;  // 直接返回
      }
      // 下载 output donelist 到 本地
      // output donelist 记录格式: output_dir \t status_data_dir \t input_file \t host_loadfile
      cmd = "${HADOOP_HOME}/bin/hadoop fs -get " + output_donelist + " " + local_output_tmp_donelist;
      if (!execute_cmd_with_retry(cmd, 3)) {  // get output donelist to local fail
        LOG(FATAL) << "Faild to execute cmd, cmd is: " << cmd;
      }
      //  会清空 output_donelist_set
      LOG_IF(FATAL, !LoadDonelistToSet(local_output_tmp_donelist, &output_donelist_set))
          << "file: " << local_output_tmp_donelist;
      // 从 done list vector 中逆向查找倒当前没有处理的最新的一个任务
      for (int i = input_donelist_vector.size()-1; i >= 0; --i) {
        const std::string &url_task = input_donelist_vector[i];
        auto it = output_donelist_set.find(url_task);
        if (it == output_donelist_set.end()) {
          std::string input_file_prefix = url_task;
          *url_file = input_file_prefix + ".url";
          *host_load = input_file_prefix + ".ipload";
          std::string::size_type pos = input_file_prefix.rfind("/");
          LOG_IF(FATAL, pos == std::string::npos) << "url_file name in donelist is not valid, filename: "
                                                  << input_file_prefix;
          *priority = i;
          *hdfs_output_dir = output_prefix + "/" + input_file_prefix.substr(pos+1) + ".output";
          *hdfs_output_status_dir = output_prefix + "/" + input_file_prefix.substr(pos+1) + ".status.st";
          *output_donelist_ret = output_donelist;
          *local_output_donelist_ret = local_output_tmp_donelist;
          *input_donelist_record = input_file_prefix;
          return true;
        }
      }
    }
  }
  return false;
}

bool SplitTaskUrl(const std::list<crawler::UrlHostIP> &task_item_list, int thread_num,
                  int concurrent_window,
                  std::vector<std::list<crawler::UrlHostIP>> *each_thread_tasks) {
  int offset = 0;
  int thread_id;
  base::PseudoRandom pr(base::GetTimestamp());
  for (auto it = task_item_list.begin(); it != task_item_list.end(); ++it) {
    const std::string &ip = it->ip;
    int tmp_id = base::CityHash64(ip.c_str(), ip.size()) % thread_num;
    offset = pr.GetInt(0, concurrent_window-1);
    thread_id = (tmp_id + offset) % thread_num;
    (*each_thread_tasks)[thread_id].push_back(*it);
  }
  return true;
}

bool AppendOutputDonelist(const std::string &local_output_donelist, const std::string &target_donelist,
                          const std::string &record) {
  FILE *fp = fopen(local_output_donelist.c_str(), "a");
  if (fp == NULL) {
    LOG(ERROR) << "open local output donelist file fail, file: " << local_output_donelist;
    return false;
  }
  size_t size = record.size();
  LOG_IF(FATAL, size != fwrite(record.c_str(), 1, size, fp))
      << "write less bytes num than expected, expect num: " << size;
  fclose(fp);

  // 上传 本地 donelist 到 hadoop
  if (!crawler::HadoopPutOneFileWithRetry(local_output_donelist, target_donelist, 3)) {
    LOG(ERROR) << "HadoopPutOneFileWithRetry() fail, local file: " << local_output_donelist
               << ", target hdfs file: " << target_donelist;
    return false;
  }
  return true;
}

// get the output dir of cralwer
bool BuildHdfsOutputDir(const std::string &hdfs_output_root_dir, std::string *page_dir, std::string *link_dir,
                  std::string *css_dir, std::string *image_dir, std::string *newlink_dir) {
  LOG_IF(FATAL, hdfs_output_root_dir.empty()) << "empty string";
  *page_dir = hdfs_output_root_dir + "/page.sf";
  *link_dir = hdfs_output_root_dir + "/link.st";
  *css_dir = hdfs_output_root_dir + "/css.sf";
  *image_dir = hdfs_output_root_dir + "/image.sf";
  *newlink_dir = hdfs_output_root_dir + "/newlink.st";
  // 创建输出目录
  std::string cmd;
  // page
  cmd = "${HADOOP_HOME}/bin/hadoop fs -test -d " + *page_dir;
  if (!execute_cmd_with_retry(cmd, 1)) {
    cmd = "${HADOOP_HOME}/bin/hadoop fs -mkdir " + *page_dir;
    if (!execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Fail, cmd: " << cmd;
      return false;
    }
  }
  // link
  cmd = "${HADOOP_HOME}/bin/hadoop fs -test -d " + *link_dir;
  if (!execute_cmd_with_retry(cmd, 1)) {
    cmd = "${HADOOP_HOME}/bin/hadoop fs -mkdir " + *link_dir;
    if (!execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Fail, cmd: " << cmd;
      return false;
    }
  }
  // css
  cmd = "${HADOOP_HOME}/bin/hadoop fs -test -d " + *css_dir;
  if (!execute_cmd_with_retry(cmd, 1)) {
    cmd = "${HADOOP_HOME}/bin/hadoop fs -mkdir " + *css_dir;
    if (!execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Fail, cmd: " << cmd;
      return false;
    }
  }
  // image
  cmd = "${HADOOP_HOME}/bin/hadoop fs -test -d " + *image_dir;
  if (!execute_cmd_with_retry(cmd, 1)) {
    cmd = "${HADOOP_HOME}/bin/hadoop fs -mkdir " + *image_dir;
    if (!execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Fail, cmd: " << cmd;
      return false;
    }
  }
  // newlink
  cmd = "${HADOOP_HOME}/bin/hadoop fs -test -d " + *newlink_dir;
  if (!execute_cmd_with_retry(cmd, 1)) {
    cmd = "${HADOOP_HOME}/bin/hadoop fs -mkdir " + *newlink_dir;
    if (!execute_cmd_with_retry(cmd, 3)) {
      LOG(ERROR) << "Fail, cmd: " << cmd;
      return false;
    }
  }
  return true;
}

// task list file format : just hdfs file name for each line
bool ParseTaskListFile(const std::string &total_task_filename, std::vector<std::string> &hdfs_files_vector) {
  LOG_IF(FATAL, total_task_filename.empty()) << "function parameter total_task_filename is empty";

  std::ifstream my_fin(total_task_filename);
  LOG_IF(FATAL, !my_fin) << "Open total_task_filename:" << total_task_filename << "fail.";
  hdfs_files_vector.clear();
  std::string line;
  while (getline(my_fin, line)) {
    base::TrimTrailingWhitespaces(&line);
    if (line.empty()) continue;
    hdfs_files_vector.push_back(line);
  }
  return true;
}

// record format like: url \t resource_type \t importance \t refer_url \t payload
// 以追加的方式写入 list, 即: 不会清除 |urls| 原有的内容
const int kRecordNum = 6;
bool ParseNewTaskFile(const std::string &filename, std::list<UrlHostIP> *urls) {
  CHECK_NOTNULL(urls);
  std::string line, url, ip, refer_url, payload;
  ResourceType resource_type;
  std::vector<std::string> tokens;
  std::ifstream myfin(filename);
  LOG_IF(FATAL, !myfin.good()) << "Open file: " << filename << " fail.";

  int num_fields;
  double importance;
  while (getline(myfin, line)) {
    if (line.empty()) continue;
    tokens.clear();
    base::SplitString(line, "\t", &tokens);
    num_fields = tokens.size();
    LOG_IF(FATAL, (num_fields != kRecordNum)) << "Records expected to have "
                                              << kRecordNum << " fields, record: "
                                              << line << ", field#: " << num_fields;
    url = tokens[0];
    ip = tokens[1];

    CHECK(!url.empty());
    CHECK(!ip.empty());

    // 前提： resource_type 取值在 0～9 可以用一个字符表示
    LOG_IF(FATAL, tokens[2].size() != 1) << "resource_type should [0,9] and be one byte, but: " << tokens[2];
    resource_type = static_cast<ResourceType>(tokens[2][0] - '0');
    LOG_IF(FATAL, !base::StringToDouble(tokens[3], &importance))
        << "Failed to convert string to double, importance: " << tokens[3];
    char from = 'K';
    char refer_from = 'K';
    refer_url = tokens[4];
    payload = tokens[5];

    GURL gurl(url);
    UrlHostIP info(url, refer_url, gurl.HostNoBrackets(), ip,
                   resource_type, importance, from, refer_from, payload);  // NOLINT
    // 如果 host 本来就是 IP 形式，则赋值给 hostinfo.ip
    if (gurl.HostIsIPAddress()) info.ip = info.site;
    urls->push_back(info);
  }
  urls->sort();
  urls->erase(unique(urls->begin(), urls->end()), urls->end());
  return true;
}

static bool CheckParameter(const ThreadParameter *para) {
  if (!para) {
    return false;
  }
  if (para->hostload_conf_file.empty()) {
    LOG(ERROR) << "hostload conf file str is empty";
    return false;
  }
  if (!para->ip_scopes || !para->statistics || !para->saver || !para->url_list) {
    LOG(ERROR) << "Null (!ip_scopes ||!statistic || !saver || !url_list)";
    return false;
  }
  return true;
}

enum HostType {kIn = 1, kOut, kGetIpTimeout, kNoIp};
struct HostInfo {
  HostType host_type;
  std::string ip;
  explicit HostInfo(HostType type): host_type(type) {}
  HostInfo(HostType type, std::string ip): host_type(type), ip(ip) {}
};

//  static void VerifyHostType(const std::vector<EdgeNode> *ip_scopes,
//                             const std::vector<HostIpInfo> *phosts,
//                             std::map<std::string, HostInfo> *pmap) {
//    CHECK(ip_scopes != NULL && phosts != NULL && pmap != NULL);
//    pmap->clear();
//    std::vector<std::string> ips;
//    base::PseudoRandom pr(base::GetTimestamp());
//    for (std::vector<HostIpInfo>::const_iterator it = phosts->begin(); it != phosts->end(); ++it) {
//      ips = it->ips;
//      if (ips.empty()) {
//        if (it->ip_state == kGetIpFailureNoIp) {
//          pmap->insert(std::map<std::string, HostInfo>::value_type(it->host, HostInfo(kNoIp)));
//        } else {  // it->ip_state == kGetIpFailureTimeOut
//          pmap->insert(std::map<std::string, HostInfo>::value_type(it->host, HostInfo(kGetIpTimeout)));
//        }
//      } else {
//        std::vector<std::string>::iterator it2;
//        for (it2 = ips.begin(); it2 != ips.end(); ++it2) {
//          if (!InCurrentIPScope(ip_scopes, *it2)) {
//            pmap->insert(std::map<std::string, HostInfo>::value_type(it->host, HostInfo(kOut)));
//            break;
//          }
//        }
//        if (it2 == ips.end()) {
//          int random = pr.GetInt(0, ips.size()-1);   // base/random/psuedo_random.h
//          pmap->insert(std::map<std::string, HostInfo>::value_type(it->host, HostInfo(kIn, ips[random])));
//        }
//      }
//    }
//  }

// static void CalculateHostFromUrlSet(std::list<UrlHostIP> *purls, std::vector<HostIpInfo> *phosts,
//                                     TaskStatistics *stat) {
//   std::string host;
//   for (std::list<UrlHostIP>::iterator it = purls->begin(); it != purls->end();) {
//     host = (*it).host;
//     if (!host.empty()) {
//       phosts->push_back(HostIpInfo(host));
//       ++it;
//     } else {
//       it = purls->erase(it);
//     }
//   }
//   // 域名去重
//   sort(phosts->begin(), phosts->end());
//   phosts->erase(unique(phosts->begin(), phosts->end()), phosts->end());
// }

DEFINE_int64_counter(crawler, total_urls_need_to_fetch_num, 0, "total urls# need to fetch(html css image)");
//  void CrawlerWorkerThread(ThreadParameter parameter) {
//  int64 thread_id = pthread_self();
//  LOG_IF(FATAL, !CheckParameter(&parameter))
//      << "Invalid parameter[CheckParameter() fail] for this thread: " << thread_id;
//  TaskParameter task = parameter.task;
//  TaskStatistics *stat = parameter.statistics;
//
//  // Create and Init CrawlerAnalyzer
//  simhash::HtmlSimHash html_simhash;
//
//  std::string local_file_name;
//  // 下载 hadoop 文件到本地
//  if (!FetchTaskFileFromHadoop(task.hadoop_file_path, parameter.worker_id, &local_file_name)) {
//    LOG(ERROR) << "FetchTaskFileFromHadoop() fail, may be this task has done!";
//    return;
//  }
//  int64 timestamp = base::GetTimestamp();
//  // parse the task file to obtain urls need to crawle
//  std::list<UrlHostIP> urls;
//  LOG_IF(FATAL, !ParseTaskFile(local_file_name, &urls))
//      << "Thread_id: " << thread_id  << ", call ParseTaskFile() fail.";
//  stat->link_total_num = static_cast<int64>(urls.size());
//  if (urls.size() == 0) {
//    LOG(ERROR) << "Thread_id: " << thread_id  << ", Input hdfs task file is empty.";
//    return;
//  }
//  // 从 url 提取域名
//  std::vector<HostIpInfo> hosts;
//  CalculateHostFromUrlSet(&urls, &hosts, stat);
//  // 解析域名，获得对应的 IP
//  DnsResolver *dns_resolver = new DnsResolver(parameter.dns_servers, 1600);
//  CHECK_NOTNULL(dns_resolver);
//  LOG_IF(FATAL, !dns_resolver->Init()) << "dns_resolver.Init() fail";
//  LOG_IF(FATAL, !dns_resolver->ResolveHostBatchMode(&hosts)) << "ResolveHostBatchMode() fail";
//  delete dns_resolver;
//  LOG(INFO) << "Finish ResolveHostBatchMode(), total host: " << hosts.size();
//  // Init Saver
//  LOG_IF(FATAL, !parameter.saver->Init()) << "Failed in Init Saver";
//  // 1, 判断 host 是否在国内，不再国内的 URLs 被过滤
//  // 2. host DNS 解析失败 (eg: Domain not found...) 的 URLs 被过滤
//  // 2012.0313: 该规则目前只对 程序从网页中提取出来的新的 Link 应用, Search Log and User Log PV log
//  // 没有使用这个规则
//  // 2012.0326: 撤销 2012.0313 规则, 即: 对所有的 URL(kHTML) 进行过滤掉 Foreign Url
//  FilterUrls(&parameter, &urls, hosts, parameter.saver);
//  // 完成了 domain 是否属于国内的判断
//  // To release memory
//  hosts.clear();
//  int64 timestamp2 = base::GetTimestamp();
//  // 检查过滤后的需要抓取的 url 数，如果为 0 ，则不需要抓取
//  if (urls.empty()) {
//    LOG(WARNING) << "Thread_id: " << thread_id << ", all urls in hdfs file: " << task.hadoop_file_path
//                 << " has been filered and the worker done nothing(because they are foreign domain).";
//  } else {
//    // 为了能够局部均匀对单个 web host 的抓取压力，使用 random shuffle 使其分布随机
//    std::vector<UrlHostIP> shuffled_urls(urls.begin(), urls.end());
//    urls.clear();
//    random_shuffle(shuffled_urls.begin(), shuffled_urls.end());
//    std::list<UrlHostIP> fetch_failed_list;
//    // 该临时文件存放 抓取超时 失败的 URLs , 避免内存膨胀
//    std::string timeout_url_tmpfile = base::StringPrintf("./tmp_%s/timeout_url_%ld.txt",
//                                                         parameter.worker_id.c_str(), thread_id);
//    system(("cat /dev/null > " + timeout_url_tmpfile).c_str());
//    crawler::MultiFetcher *multi_fetcher =
//        new crawler::MultiFetcher(stat, parameter.saver, parameter.dns_servers, &fetch_failed_list,
//                                  timeout_url_tmpfile, 600, &html_simhash);
//    CHECK_NOTNULL(multi_fetcher);
//    LOG_IF(FATAL, !multi_fetcher->Init()) << "MultiFetcher Init() fail";
//    // cralwer begin to crawle all those urls: may cost a lot of time
//    COUNTERS_crawler__total_urls_need_to_fetch_num.Increase(shuffled_urls.size());
//    LOG_IF(FATAL, !multi_fetcher->MultiCrawlerAndAnalyze(&shuffled_urls))
//        << "Thread_id: " << thread_id << ", Failed In MultiCrawlerAndAnalyze().";
//    // 处理抓取超时 URLs: 对获取失败的 link ，进行 |times_retry| 次重试
//    int counter = 0;
//    // const int times_retry = 0;  // do not retry for timeout url
//    const int times_retry = 0;  // do not retry for timeout url
//    int64 retry_fetcher_ok;
//    int64 filesize = 0;
//    base::FilePath filepath(timeout_url_tmpfile);
//    while (true) {
//      shuffled_urls.clear();
//      if (counter >= times_retry) break;
//      if (!base::file_util::GetFileSize(filepath, &filesize)) {
//        LOG(ERROR) << "GetFileSize() fail, filepath: " << timeout_url_tmpfile;
//        return;
//      }
//      if (filesize > 0) {
//        if (!ParseTaskFile(timeout_url_tmpfile, &fetch_failed_list)) {
//          LOG(ERROR) << "ParseTaskFile() for timeout urls fail: " << timeout_url_tmpfile;
//          return;
//        }
//      }
//      if (fetch_failed_list.empty()) break;
//      stat->link_fetch_timeout_num = 0;
//      retry_fetcher_ok = stat->link_fetch_ok_num + stat->css_fetch_num + stat->image_fetch_num;
//      for (std::list<UrlHostIP>::const_iterator it = fetch_failed_list.begin();
//           it != fetch_failed_list.end(); ++it) {
//        shuffled_urls.push_back(*it);
//      }
//      // To release memroy
//      fetch_failed_list.clear();
//      system(("cat /dev/null > " + timeout_url_tmpfile).c_str());
//      random_shuffle(shuffled_urls.begin(), shuffled_urls.end());
//      LOG_IF(FATAL, !multi_fetcher->MultiCrawlerAndAnalyze(&shuffled_urls))
//          << "Thread_id: " << thread_id << ", Failed In retry MultiCrawlerAndAnalyze().";
//      ++counter;
//      LOG(INFO) << "Retry the " << counter << "times: " << "retried link#" << shuffled_urls.size()
//                << ", fetched OK#"
//              << (stat->link_fetch_ok_num + stat->css_fetch_num + stat->image_fetch_num - retry_fetcher_ok);
//    }
//
//    multi_fetcher->ResourceRelease();
//    delete multi_fetcher;
//  LOG_IF(WARNING, !base::file_util::Delete(filepath, true)) << "Delete file fail: " << timeout_url_tmpfile;
//  }
//  parameter.saver->ResourceRelease();
//  // We need to clear buffer here to release memroy
//  // 抓取完一个任务后，修改其 hadoop 文件名字为 done_ + 原来的名字
//  LOG_IF(WARNING, !RenameFinishHdfsFile(task.hadoop_file_path)) << "Move file to 'done_'+filename fail";
//  base::FilePath file_path(local_file_name);
//  LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete local file fail";
//
//  int64 timestamp3 = base::GetTimestamp();
//  double total_time = (timestamp3-timestamp) * 1.0 / 1000 / 1000;
//  LOG(INFO) << "\nDONE: Thread_id: " << thread_id <<", finish crawle file: " << task.hadoop_file_path
//            << ", Total cost time: " << total_time << "s (range checking: "
//            << (timestamp2 - timestamp) * 1.0 / 1000 / 1000 << "s, crawle: "
//            << (timestamp3 - timestamp2) * 1.0 / 1000 / 1000 << "s, total_link to fetch#="
//            << stat->link_total_num << ", fetched html#=" << stat->link_fetch_ok_num << "fetched css#="
//            << stat->css_fetch_num << ", fetched image#=" << stat->image_fetch_num << "fetched timeout#="
//            << stat->link_fetch_timeout_num << ", QPS="
//            << (stat->link_fetch_ok_num + stat->css_fetch_num + stat->image_fetch_num) / total_time;
//  }

void CrawlerWorkerThreadNew(ThreadParameter parameter) {
  int64 thread_id = pthread_self();
  LOG_IF(FATAL, !CheckParameter(&parameter))
      << "Invalid parameter[CheckParameter() fail] for this thread: " << thread_id;
  TaskStatistics *stat = parameter.statistics;
  std::list<UrlHostIP> *urls = parameter.url_list;

  int64 timestamp = base::GetTimestamp();

  //  // 从 url 提取域名
  //  std::vector<HostIpInfo> hosts;
  //  LOG(INFO) << "Getting host set from urls";
  //  CalculateHostFromUrlSet(urls, &hosts, stat);
  //  // 解析域名，获得对应的 IP
  //  DnsResolver *dns_resolver = new DnsResolver(parameter.dns_servers, 1600);
  //  CHECK_NOTNULL(dns_resolver);
  //  LOG_IF(FATAL, !dns_resolver->Init()) << "dns_resolver.Init() fail";
  //  LOG_IF(FATAL, !dns_resolver->ResolveHostBatchMode(&hosts)) << "ResolveHostBatchMode() fail";
  //  delete dns_resolver;
  //  hosts.clear();
  //  LOG(INFO) << "Finish ResolveHostBatchMode(), total host: " << hosts.size();

  // Init Saver
  LOG_IF(FATAL, !parameter.saver->Init()) << "Failed in Init Saver";

  // 1, 判断 host 是否在国内，不再国内的 URLs 被过滤
  // 2. host DNS 解析失败 (eg: Domain not found...) 的 URLs 被过滤
  // 2012.0313: 该规则目前只对 程序从网页中提取出来的新的 Link 应用, Search Log and User Log PV log
  // 没有使用这个规则
  // 2012.0326: 撤销 2012.0313 规则, 即: 对所有的 URL(kHTML) 进行过滤掉 Foreign Url
  // FilterUrls(&parameter, urls, hosts, parameter.saver);
  // 完成了 domain 是否属于国内的判断
  int64 timestamp2 = base::GetTimestamp();
  // 检查过滤后的需要抓取的 url 数，如果为 0 ，则不需要抓取
  if (urls->empty()) {
    LOG(WARNING) << "Thread_id: " << thread_id << ", no urls";
  } else {
    // 为了能够局部均匀对单个 web host 的抓取压力，使用 random shuffle 使其分布随机
    std::vector<UrlHostIP> shuffled_urls(urls->begin(), urls->end());
    urls->clear();
    random_shuffle(shuffled_urls.begin(), shuffled_urls.end());
    crawler::MultiFetcher *multi_fetcher =
        new crawler::MultiFetcher(stat, parameter.saver, parameter.dns_servers,
                                  600, parameter.hostload_conf_file,
                                  parameter.max_concurrent_access_fetcher);
    CHECK_NOTNULL(multi_fetcher);
    LOG_IF(FATAL, !multi_fetcher->Init()) << "MultiFetcher Init() fail";
    // cralwer begin to crawle all those urls: may cost a lot of time
    COUNTERS_crawler__total_urls_need_to_fetch_num.Increase(shuffled_urls.size());
    LOG_IF(FATAL, !multi_fetcher->MultiCrawlerAndAnalyze(&shuffled_urls))
        << "Thread_id: " << thread_id << ", Failed In MultiCrawlerAndAnalyze().";

    multi_fetcher->ResourceRelease();
    delete multi_fetcher;
  }
  parameter.saver->ResourceRelease();
  // 每个线程负责释放 自己使用的资源
  delete parameter.saver;

//  // We need to clear buffer here to release memroy
//  base::FilePath file_path(local_file_name);
//  LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete local file fail";

  int64 timestamp3 = base::GetTimestamp();
  double total_time = (timestamp3-timestamp) * 1.0 / 1000 / 1000;
  LOG(INFO) << "Done, thread_id: " << thread_id
            << ", total cost time: " << total_time << "s, (IP range check: "
            << (timestamp2 - timestamp) * 1.0 / 1000 / 1000 << "s, crawl: "
            << (timestamp3 - timestamp2) * 1.0 / 1000 / 1000 << "s), total link to fetch #: "
            << stat->link_total_num << ", fetched html #: " << stat->link_fetch_ok_num << ", fetched css #: "
            << stat->css_fetch_num << ", fetched image #: " << stat->image_fetch_num << ", timeout #: "
            << stat->link_fetch_timeout_num << ", QPS: "
            << (stat->link_fetch_ok_num + stat->css_fetch_num + stat->image_fetch_num) / total_time;
  // 每个线程负责释放 自己使用的资源
  delete stat;
}
}  // namespace crawler
