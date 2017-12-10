// 文件 page_crawler.h 实现了一个用于抓取简单网页的接口
// examples:
// ....
// PageCrawler crawler;
// crawler.Init();
// CrawlTask task;
// task.url = "http://....";
// CrawlRoutineStatus status;
// Resource res;
// crawler.AddTask(task, &status, &res, NULL);
// crawler.WaitForCompleted();

#pragma once

#include <curl/curl.h>
#include <curl/multi.h>
#include <sys/epoll.h>

#include <cstdatomic>
#include <deque>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "base/common/basic_types.h"
#include "base/thread/sync.h"
#include "base/thread/notify_queue.h"
#include "crawler/crawl/status_code.h"
#include "crawler/crawl/crawl_task.h"
#include "crawler/crawl/data_io.h"

namespace web {
class UrlNorm;
}  // namespace web

namespace crawler2 {
namespace crawl {
// 此类负责抓取网页
// 此类不支持多个实例同时运行
class PageCrawler {
 public:
  struct Options {
    int connect_timeout_in_seconds;
    int transfer_timeout_in_seconds;
    int low_speed_limit_in_bytes_per_second;
    int low_speed_duration_in_seconds;
    std::string user_agent;
    std::string user_agent_p;

    // 0: 表示不 follow, 对于图片不需要 follow
    int max_redir_times;
    int max_fetch_times_to_reset_cookie;
    // 处理数据的线程数量
    bool enable_curl_debug;
    // 需要进行 hack 特殊处理的 urls 列表文件名
    std::string client_redirect_urls_filepath;
    // 是否只请求头,即: HEAD 请求
    // 默认为 0, 即 GET/POST
    // 1 表示 HEAD 请求
    int nobody;
    Options()
        : connect_timeout_in_seconds(120)
        , transfer_timeout_in_seconds(120)
        , low_speed_limit_in_bytes_per_second(50)
        , low_speed_duration_in_seconds(20)
        , user_agent("360Spider")
        , user_agent_p("Mozilla/5.0")
        , max_redir_times(10)
        , max_fetch_times_to_reset_cookie(50)
        , enable_curl_debug(false)
        , nobody(0) {
    }
  };

  explicit PageCrawler(const Options& option);
  ~PageCrawler();
  // 初始化
  void Init();

  void AddTask(CrawlTask* task, CrawlRoutineStatus* status,
               Resource* res, Closure* done);

  void AddMultiTask(std::vector<CrawlTask*>* tasks,
                    std::vector<CrawlRoutineStatus*>* status,
                    std::vector<Resource*>* resources, Closure* done);

  // 在所有任务完成之后，返回
  void WaitForCompleted();

  // 队列当中的任务是否全部完成
  bool IsAllTasksCompleted() const;

  // 抓取任务的历程，
  void CheckTaskStatus();
 private:
  void SingleTaskDone(WholeTask* task);

  struct CURLData {
    std::string head;
    std::string body;
    curl_slist *request_header;
    curl_slist *host_ip;

    CURLData()
        : request_header(NULL)
        , host_ip(NULL) {
      }

    ~CURLData() {
      if (request_header != NULL) {
        curl_slist_free_all(request_header);
      }

      if (host_ip != NULL) {
        curl_slist_free_all(host_ip);
      }
    }
  };

  // 包含抓取的完整数据
  // 数据伴随一条抓取记录的完整流程
  // 此数据结构为内部使用
  struct CrawlRoutineData {
    CrawlTask* task;
    CrawlRoutineStatus* status;
    Resource* res;
    Closure* done;
    CURLData private_data;
    CrawlRoutineData(CrawlTask* t, CrawlRoutineStatus* s,
                     Resource* r, Closure* d)
        : task(t)
        , status(s)
        , res(r)
        , done(d) {
    }

    ~CrawlRoutineData() {
    }
  };

  ::thread::NotifyQueue<CrawlRoutineData*> tasks_;
  std::atomic<bool> thread_started_;
  std::atomic<int> current_tasks_num_;

  static int CallbackBodyData(char* data, size_t num, size_t unit_size, void* user_data);
  static int CallbackHeadData(char* data, size_t num, size_t unit_size, void* user_data);
  static int CallbackSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
  static int CallbackTimer(CURLM *cm, int64 timeout_ms, void *userp);

  void SetEasyHandler(CURL *eh, CrawlRoutineData *routine);
  void StartFetchTask(CrawlRoutineData* routine);
  void OnFinishTask(CURLMsg* msg);
  CURLM* GetMultiHandle(CrawlRoutineData *routine);
  bool ExtractInfoFromCurl(CURLMsg *msg, CrawlRoutineData *routine);

  std::atomic<bool> stopped_;
  Options options_;
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
  int64 epoll_timeout_ms_;

  // 存放需要支持 https 协议的 urls
  std::unordered_set<std::string> https_supported_urls_;
  // 存放需要 hack 的 urls
  // XXX(pengdan): 对 |client_redirect_urls| 的 url 做如下特殊处理:
  // 1. source url 和 effective url hack 成 map 中的 key;
  // 2. 实际抓取的网页, 却是 对应 map 中 value url 的内容
  std::unordered_map<std::string, std::string> client_redirect_urls_;

 private:
  void SetProxyUserAgent(CURL *eh, const CrawlTask *task);
};

}  // namespace crawl
}  // namespace crawler
