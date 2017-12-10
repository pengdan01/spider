#pragma once

#include <curl/curl.h>
#include <curl/multi.h>

#include <deque>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/common/basic_types.h"
#include "base/time/timestamp.h"
#include "base/common/logging.h"
#include "crawler/fetcher2/load_controller.h"
#include "crawler/fetcher2/fetcher_task.h"

namespace web {
class UrlNorm;
}

namespace crawler2 {

// 给定 url 集合，和 ip load 参数，抓取所有 url
//
// 该类 不是 thread-safe 的, 且每个进程只能有一个实例。
class Fetcher {
 public:
  struct Options {
    std::string user_agent;  // 爬虫发给 web server 的 user agent
    int max_redir_times;       // 允许 HTTP 重定向的次数
    std::unordered_set<std::string> https_urls;  // 仅该集合的 url 可 指向 或 重定向 到 https 页面
    std::vector<std::string> dns_servers;  // 可供使用的 dns 服务器, 如果不设置, 则使用系统默认 dns
    std::vector<std::string> proxy_servers;  // 可供使用的代理服务器
    bool enable_curl_debug;  // 是否开启 curl 的调试选项, 会把 http 会话输出到控制台

    // 建立连接时长上限
    int connect_timeout_in_seconds;
    // 传输数据时长上限
    int transfer_timeout_in_seconds;
    // 慢连接速度上限
    int low_speed_limit_in_bytes_per_second;
    // 慢连接持续时间上限
    int low_speed_duration_in_seconds;

    // 设置一个上限, 按调度规则, 最多选取 max_fetch_num 个 url 发起抓取,
    // 小于等于 0 表示不设限制; 该选项仅供调试使用.
    int64 max_fetch_num;

    // 使用 proxy 上的 dns 解析，
    bool use_proxy_dns;

    // 每个代理上, 连续抓取失败的次数上限 超过该限制, 该代理会被认为已失效,
    // 会将其从候选代理中删除
    //
    // 超过一半的代理失效, 程序会 crash, 要求人工介入检查代理设置
    int proxy_max_successive_failures;

    bool always_create_new_link;
    Options() {
      connect_timeout_in_seconds = 0;
      transfer_timeout_in_seconds = 0;
      low_speed_limit_in_bytes_per_second = 0;
      low_speed_duration_in_seconds = 0;
      max_redir_times = 10;
      enable_curl_debug = false;
      max_fetch_num = 0;
      use_proxy_dns = false;
      always_create_new_link = false;
      proxy_max_successive_failures = 0;
    }
  };

  struct FetchedUrlCallback {
    // url_fetched 的 ownership 归 callback, 由 callback 负责删除
    virtual void Process(UrlFetched *url_fetched) = 0;
    virtual ~FetchedUrlCallback() {}
  };

  Fetcher(const Options &options, LoadController &load_controller);
  ~Fetcher();

  // 阻塞调用, 获取所有网页, 直到全部结束
  //
  // 如果初始化失败，则崩溃
  //
  // 每次抓取一个 url, 调用一次 callback.Process();
  //
  // NOTE:
  // 1. urls_to_fetch 会被修改, 每次发起一个抓取请求, 则将对应的 UrlToFetch 对象从 deque 中删除.
  // 2. url 抓取请求的 发起 和 结束 的顺序是随机的, 跟 urls_to_fetch 里的顺序无关.
  void FetchUrls(std::deque<UrlToFetch> *urls_to_fetch, FetchedUrlCallback *callback);
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

  // 从待抓列表里, 选取一个可抓的 url (考虑 压力 控制)
  //
  // 传入上一次使用的元素下标, 返回新选中元素及其下标
  // 返回值
  // 0: 成功找到待抓 URL
  // 1: 找到需要丢弃的 URL
  // 2: 未找到待抓 URL
  int ObtainUrl(std::deque<UrlToFetch> *urls_to_fetch, int *last_fetch_index, UrlToFetch *url_to_fetch);

  void StartFetchUrl(const UrlToFetch &url_to_fetch);
  void FinishFetchUrl(CURLMsg *msg, FetchedUrlCallback *callback);
  void DiscardFetchUrl(const UrlToFetch &url_to_fetch, FetchedUrlCallback *callback);

  void SetEasyHandler(CURL *eh, const UrlToFetch &url_to_fetch);
  bool ExtractInfoFromCurl(CURLMsg *msg, const PrivateData *private_data, UrlFetched *url_fetched);

  void AllocateMultiHandlers(const std::deque<UrlToFetch> &urls_to_fetch);
  CURLM *GetMultiHandle(const UrlToFetch &url_to_fetch);

  const Options options_;
  LoadController &load_controller_;

  web::UrlNorm *url_norm_;

  // XXX(suhua): 这里使用多个 curl multi handler, 为了绕过 libcurl dns cache 的
  // bug, 详情如下:
  //
  // 1. 我们希望可以由外部指定每个 url 的 host 解析到的 ip, 从而可以将一个 host
  // 的访问压力分散到多个 ip 上
  // 2. 最初的实现方式是利用 curl 的 dns cache, 可以手工将特定的 host -> ip
  // 映射添加到 dns cache 中去, 这样当 curl 就会用设定的 ip 去访问站点.
  // 3. 但是该 dns cache 对一个 multi handlers 的所有 easy handler 生效, 并且有
  // bug, 不能正确的重置 dns cache. 症状是: 有时 host 实际上只有一个 ip 生效;
  // 有时会忽略设置的 ip, 而是请求 dns 去解析 ip, 由于部分站点使用了 dns
  // 轮询技术, 此时还能看到同一 host 的多个生效 ip 随机出现,
  // 但是跟程序里设定的不一样, 非常具有迷惑性.
  // 4. 最简单的解法, 就是用多个 multi handler, 并将一个 host 的多个 ip, 每个
  // ip 分配到不同的 multi handler. 这样每个 multi handler 里的 host
  // 只需要设置一次 host->ip, 也就不需要重置了.
  // 5. 需要一个简单的算法, 将同一 host 的多个 ip, 分配到不同的 multi handler 上
  // 6. 由于潜在的性能问题, 不能建太多的 multi handler. 如果一个 host 的 ip
  // 太多超过 multi handler 的数量, 会有部分 ip 被忽略.
  //
  // 详情参见成员函数 AllocateMultiHandlers() 和 GetMultiHandle();
  std::vector<CURLM *> curl_multi_handlers_;
  int epoll_fd_;
  int epoll_timeout_ms_;

  std::unordered_map<std::string, int> ip_handle_id_map_;

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
