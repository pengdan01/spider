#pragma once
#include <vector>
#include <list>
#include <string>
#include <set>
#include <map>

#include "base/common/basic_types.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/fetcher/resource_saver.h"
#include "crawler/fetcher/crawler_analyzer.h"
#include "crawler/exchange/report_status.h"

namespace crawler {

struct PrivateData;
struct HostLoadInfo;
struct HostLoadControl;
// a byte buffer sturct
struct ByteBuffer {
  char *start;
  int len;
  ByteBuffer(char *buf = NULL, int len = 0): start(buf), len(len) {}
};

class MultiFetcher {
 public:
  MultiFetcher(TaskStatistics *stat, ResourceSaver *saver, const std::string &dns_servers,
               int request_window_size, const std::string &hostload_conf_file,
               int max_concurrent_access_fetcher);
  ~MultiFetcher() {}

  bool Init();
  void ResourceRelease();
  // |url| 为用户需要抓取的 urls
  // |http_header| 存放 抓取网页的 HTTP 协议头
  // |http_body| 存放 抓取网页的 body
  // |debug| 是否显示抓取详细信息，用于 debug
  //
  // 成功返回 true
  // 失败返回 false
  // NOTE(pengdan): 该函数目前 仅 支持 http 协议. 如果您需要支持更多的协议, 请和该模块 OWNER 联系.
  static bool SimpleFetcher(const std::string &url, std::string *http_header, std::string *http_body,
                            bool debug);
  static bool LoadHostControlRules(const std::string &rule_file,
                                   std::map<std::string, std::vector<HostLoadControl>> *site_access_paras,
                                   int max_concurrent_access_fetcher);
  static void GetHostLoadParameter(const std::map<std::string, std::vector<HostLoadControl>> *site_access_paras,  // NOLINT
                                const std::string &host, int *max_curr, int *max_qps, int *internal_id,
                                int max_concurrent_access_fetcher);
  bool MultiCrawlerAndAnalyze(std::vector<UrlHostIP> *urls);
 private:
  void HandleCurlMessage(CURLMsg *msg);
  void InitEasyInterface(const UrlHostIP &url_info);
  bool ExtractInfoFromCurl(CURL *curl_handle, CrawledResource *info);
  void RealtimeHostLoadAdjust(const std::string &host);
  void InitUrlSetSupportHttps(const std::string &url_file_support_https);
  void InitUrlSetNeedProxy(const std::string &url_file_need_proxy);

  CURLM *curl_multi_handler_;
  char *page_head_buffer_;
  ByteBuffer *page_head_;
  char *page_body_buffer_;
  ByteBuffer *page_body_;
  bool *buffer_free_flag_;
  int epollfd_;
  int overide_buffer_id_;

  TaskStatistics *stat_;
  ResourceSaver *saver_;

  // 压力控制配置文件
  std::string hostload_conf_file_;
  // map 存放 host 访问统计信息, 用于过载保护
  std::map<std::string, HostLoadInfo> *host_access_stats_;
  // map 存放 host 的访问压力控制参数
  std::map<std::string, std::vector<HostLoadControl>> *host_access_paras_;

  // 对于同一个 host, 最多由 |max_concurrent_access_fetcher| 个 线程同时访问
  int max_concurrent_access_fetcher_;

  int request_window_size_;
  int64 timeout_;
  std::vector<std::string> dns_servers_;

  // 存放需要同时支持 http 和 https 协议的 url 集合
  std::set<std::string> *url_set_support_https_;
  // 存放需要使用代理访问的 url 集合
  std::set<std::string> *url_set_need_proxy_;
};

// 报告 DNS 解析过程中的状态
void ReportDnsResolveStatus(ResourceSaver *saver, const std::string &url, const std::string &reason,
                            const std::string &payload);
// 报告抓取过程中的状态
void ReportCrawlStatus(ResourceSaver *saver, int libcurl_ret_code, int http_response_code,
                       const std::string &url, int64 start_timepoint,
                       int redir_times, const std::string &effective_url, const std::string &payload);
}  // namespace crawler
