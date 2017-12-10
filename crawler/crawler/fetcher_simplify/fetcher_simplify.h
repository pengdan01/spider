#pragma once

#include <curl/curl.h>
#include <curl/multi.h>

#include <deque>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/thread/blocking_queue.h"
#include "crawler/fetcher_simplify/fetcher_task.h"
#include "crawler/fetcher_simplify/fetcher.h"

using thread::BlockingQueue;
namespace crawler2 {
// 给定 url 集合，抓取所有 url
//
class FetcherSimplify {
 public:
  // XXX(pengdan) 不带参数的构造函数, 不支持使用代理抓取. 如果需要使用代理抓取, 请使用
  // 待 Fetcher::Options 的构造函数, 并需要设置 Fetcher::Options 的 proxy_servers 成员
  FetcherSimplify();
  FetcherSimplify(const Fetcher::Options &fetcher_options);
  ~FetcherSimplify();
  // 其他线程可以通过该队列发送新的抓取任务给 fetcher
  BlockingQueue<UrlToFetch> *GetRequestQueue();
  // 其他线程可以通过该队列获取抓取结果
  BlockingQueue<UrlFetched> *GetResponseQueue();

  // 调用该方法，开始运行 io 循环，获取任务和抓取
  // XXX: 批量抓取
  // void RunLoopBatch();
  //
  // 调用该方法，开始运行 io 循环，获取任务和抓取
  // XXX: 只要队列中还有待抓取 url 且 没有超过最大并发窗口, 就会打满并发窗口
  void RunLoop();
 private:
  BlockingQueue<UrlToFetch> *request_queue_;
  BlockingQueue<UrlFetched> *response_queue_;  
  Fetcher *fetcher_;
};

class FetchedCallback: public Fetcher::FetchedUrlCallback {
 public:
  explicit FetchedCallback(BlockingQueue<UrlFetched> *out_queue) {
    out_queue_ =  out_queue;
  }

  virtual ~FetchedCallback() {}
  // fetcher 每次抓完一个 url, 会调用一次 FetchedUrlCallback::Process() 方法.
  virtual void Process(crawler2::UrlFetched *url_fetched) {
    if (!out_queue_->Put(*url_fetched)) {
      LOG(ERROR) << "write crawl result to response queue fail, it is closed?";
    } else {
      LOG_EVERY_N(ERROR, 100) << "write crawl result to response queue ok(log every 100)";
    }
    delete url_fetched;
  }

 private:
  BlockingQueue<UrlFetched> *out_queue_;
};
}  // namespace crawler2
