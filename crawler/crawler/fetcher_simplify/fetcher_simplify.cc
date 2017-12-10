#include "crawler/fetcher_simplify/fetcher_simplify.h"
#include "base/thread/thread_util.h"

namespace crawler2 {

  FetcherSimplify::FetcherSimplify() {
    // 1. allocate blocking queue
    request_queue_ = new BlockingQueue<UrlToFetch>();
    CHECK_NOTNULL(request_queue_);
    response_queue_ = new BlockingQueue<UrlFetched>();
    CHECK_NOTNULL(response_queue_);
    // 2. allocate fetcher;
    Fetcher::Options options;
    options.user_agent = "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.31 (KHTML, like Gecko) Chrome/26.0.1410.64 Safari/537.31";
    options.max_redir_times = 32;
    //options.dns_servers.push_back("8.8.8.8");
    //options.dns_servers.push_back("8.8.4.4");
    options.connect_timeout_in_seconds = 10;
    options.transfer_timeout_in_seconds = 120;
    options.proxy_max_successive_failures = 100;
    options.concurrent_request_num= 1200;
    
    fetcher_ = new Fetcher(options);
    CHECK_NOTNULL(fetcher_);
  }

  FetcherSimplify::FetcherSimplify(const Fetcher::Options &options) {
    // 1. allocate blocking queue
    request_queue_ = new BlockingQueue<UrlToFetch>();
    CHECK_NOTNULL(request_queue_);
    response_queue_ = new BlockingQueue<UrlFetched>();
    CHECK_NOTNULL(response_queue_);
    // 2. allocate fetcher;
    fetcher_ = new Fetcher(options);
    CHECK_NOTNULL(fetcher_);
  }

  FetcherSimplify::~FetcherSimplify() {
    delete request_queue_;
    delete response_queue_;
    delete fetcher_;
  }

  BlockingQueue<UrlToFetch> * FetcherSimplify::GetRequestQueue() {
    return request_queue_;
  }

  BlockingQueue<UrlFetched> * FetcherSimplify::GetResponseQueue() {
    return response_queue_;
  }
/*
  void FetcherSimplify::RunLoopBatch() {
    // 1. 读取请求
    // 2. 抓取
    // 3. 写回 response queue
    thread::SetCurrentThreadName("RunLoopBatch");
    std::deque<UrlToFetch> to_fetch_list;
    FetchedCallback callback(response_queue_);
    while (request_queue_->MultiTake(&to_fetch_list) != -1) {
      fetcher_->FetchUrls(&to_fetch_list, &callback);
      // 清空 deque
      to_fetch_list.clear();
    }
  }
*/
  void FetcherSimplify::RunLoop() {
    // 1. 读取请求
    // 2. 抓取
    // 3. 写回 response queue
    thread::SetCurrentThreadName("RunLoop");
    FetchedCallback callback(response_queue_);
    fetcher_->FetchUrls(request_queue_, &callback);
  }
}
