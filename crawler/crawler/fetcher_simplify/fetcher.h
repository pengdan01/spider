#pragma once

#include <curl/curl.h>
#include <curl/multi.h>

#include <deque>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/time/timestamp.h"
#include "base/common/logging.h"
#include "base/thread/blocking_queue.h"
#include "crawler/fetcher_simplify/fetcher_task.h"

namespace crawler2 {

// 给定 url 集合，和 ip load 参数，抓取所有 url
//
// 该类 不是 thread-safe 的, 且每个进程只能有一个实例。
class Fetcher {
 public:
  struct Options {
    std::string user_agent;  // 爬虫发给 web server 的 user agent
    int max_redir_times;       // 允许 HTTP 重定向的次数
    // std::vector<std::string> dns_servers;  // 可供使用的 dns 服务器, 如果不设置, 则使用系统默认 dns
    std::vector<std::string> proxy_servers;  // 可供使用的代理服务器
    bool enable_curl_debug;  // 是否开启 curl 的调试选项, 会把 http 会话输出到控制台

    // 建立连接时长上限
    int connect_timeout_in_seconds;
    // 传输数据时长上限
    int transfer_timeout_in_seconds;
    // 设置一个上限, 按调度规则, 最多选取 max_fetch_num 个 url 发起抓取,
    // 小于等于 0 表示不设限制; 该选项仅供调试使用.
    int64 max_fetch_num;

    // 每个代理上, 连续抓取失败的次数上限 超过该限制, 该代理会被认为已失效,
    // 会将其从候选代理中删除
    //
    // 超过一半的代理失效, 程序会 crash, 要求人工介入检查代理设置
    int proxy_max_successive_failures;
    // 并发请求窗口大小
    int concurrent_request_num;

    Options() {
      connect_timeout_in_seconds = 0;
      transfer_timeout_in_seconds = 0;
      max_redir_times = 10;
      enable_curl_debug = false;
      max_fetch_num = 0;
      proxy_max_successive_failures = 0;
      concurrent_request_num = 1500;
    }
  };

  struct FetchedUrlCallback {
    // url_fetched 的 ownership 归 callback, 由 callback 负责删除
    virtual void Process(UrlFetched *url_fetched) = 0;
    virtual ~FetchedUrlCallback() {}
  };

  Fetcher(const Options &options);
  ~Fetcher();

  // 阻塞调用, 获取所有网页, 直到全部结束
  //
  // 如果初始化失败，则崩溃
  //
  // 每次抓取一个 url, 调用一次 callback.Process();
  //
  // NOTE:
  // 1. urls_to_fetch 会被修改, 每次发起一个抓取请求, 则将对应的 UrlToFetch 对象从 deque 中删除.
  void FetchUrls(std::deque<UrlToFetch> *urls_to_fetch,
                 FetchedUrlCallback *callback);
  void FetchUrls(thread::BlockingQueue<UrlToFetch> *urls_to_fetch,
                 FetchedUrlCallback *callback);
 private:
  // 供抓取过程中使用的各种临时数据
  struct PrivateData {
    UrlToFetch url_to_fetch;

    int64 start_time;

    std::string head;
    std::string body;
    curl_slist *request_header;
    curl_slist *host_ip;

    // 记录使用了的代理服务器
    std::string proxy_server_used;

    PrivateData() {
      request_header = NULL;
      host_ip = NULL;
    }
    ~PrivateData() {
      if (request_header != NULL) {
        curl_slist_free_all(request_header);
      }
      if (host_ip != NULL) {
        curl_slist_free_all(host_ip);
      }
    }
  };

  static int CallbackBodyData(char* data, size_t num, size_t unit_size, void* user_data);
  static int CallbackHeadData(char* data, size_t num, size_t unit_size, void* user_data);
  static int CallbackSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
  static int CallbackTimer(CURLM *cm, int64 timeout_ms, void *userp);

  // NOTE: urls_to_fetch 会被修改
  void Loop(std::deque<UrlToFetch> *urls_to_fetch, FetchedUrlCallback *callback);
  void Loop(thread::BlockingQueue<UrlToFetch> *urls_to_fetch_queue, FetchedUrlCallback *callback);

  // 从待抓列表里, 选取一个可抓的 url
  // 返回值
  // 0: 成功找到待抓 URL
  // 1: |urls_to_fetch| 没有元素
  int ObtainUrl(std::deque<UrlToFetch> *urls_to_fetch, UrlToFetch *url_to_fetch);

  void StartFetchUrl(const UrlToFetch &url_to_fetch);
  void FinishFetchUrl(CURLMsg *msg, FetchedUrlCallback *callback);
  void DiscardFetchUrl(const UrlToFetch &url_to_fetch, FetchedUrlCallback *callback);

  // void SetEasyHandlerForClientRedirect(CURL *eh, const UrlToFetch &url_to_fetch);
  void SetEasyHandler(CURL *eh, const UrlToFetch &url_to_fetch);
  bool ExtractInfoFromCurl(CURLMsg *msg, const PrivateData *private_data, UrlFetched *url_fetched);

  const Options options_;

  CURLM *curl_multi_handler_;
  int epoll_fd_;
  int64 epoll_timeout_ms_;

  // proxy -> <started, finished>
  struct ProxyStat {
    std::string proxy_server;
    int64 started;       // 发起的请求
    int64 finished;      // 结束的请求
    int64 connections;   // 当前连接数 (started - finished)
    int successive_failures;  // 通过该代理抓取连续失败的次数

    ProxyStat() {
      started = 0;
      finished = 0;
      connections = 0;
      successive_failures = 0;
    }
  };
  std::unordered_map<std::string, ProxyStat> proxy_stat_list_;

  const std::string &SelectBestProxy() const;
};

}  // namespace crawler2
