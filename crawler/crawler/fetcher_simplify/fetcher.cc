#include <signal.h>
//#include <sys/epoll.h>

#include <algorithm>
#include <map>

#include "base/common/scoped_ptr.h"
#include "crawler/fetcher_simplify/fetcher.h"
#include "third_party/curl_wrapper/thread_safe_curl.h"

namespace crawler2 {

const int kMaxEpollEvent = 1024;

Fetcher::Fetcher(const Options &options): options_(options) {
  epoll_fd_ = 0;
  epoll_timeout_ms_ = 100;
}

Fetcher::~Fetcher() {
  // 清理环境
  //curl_global_cleanup();
}

/*
// using epoll()
void Fetcher::FetchUrls(thread::BlockingQueue<UrlToFetch> *urls_to_fetch_queue,
                        FetchedUrlCallback *callback) {
  // 1. 初始化 curl multi handler
  // ------------------------------
  CHECK_NOTNULL(urls_to_fetch_queue);
  CHECK_EQ(thread_safe_curl_global_init(), CURLE_OK);

  PLOG_IF(FATAL, (epoll_fd_ = epoll_create(kMaxEpollEvent)) == -1) << "failed to create epoll fd";
  VLOG(3) << "create epoll fd: " << epoll_fd_;

  curl_multi_handler_ = curl_multi_init();
  CHECK_NOTNULL(curl_multi_handler_);

  // 由 epoll fd 监听 multi handler 的事件
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETFUNCTION, CallbackSocket));
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETDATA, &epoll_fd_));

  // 通过 callback 获取 epoll 事件的超时时间
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERFUNCTION, &CallbackTimer));
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERDATA, &epoll_timeout_ms_));

  // 显示忽略掉 SIGPIP 信号，因为在 DNS 异步解析时可能会产生这个信号(即使设置了 CURLOPT_NOSIGNAL)
  CHECK_NE(signal(SIGPIPE, SIG_IGN), SIG_ERR);

  // 初始化代理状态
  proxy_stat_list_.clear();
  for (auto it = options_.proxy_servers.begin(); it != options_.proxy_servers.end(); ++it) {
    // XXX(suhua): 为每一个代理初始化一个空的状态, 这样后面 SelectBestProxy()
    // 里挑选代理时, 就可以直接 proxy_stat_list_, 而不需要遍历
    // options_.proxy_servers, 然后到 proxy_stat_list_ 里查表
    proxy_stat_list_[*it].proxy_server = *it;
  }

  // 3. 开始循环抓取, 不会退出, 直到队列关闭
  // ------------------------------
  LOG(ERROR) << "before loop in fetchurl";
  Loop(urls_to_fetch_queue, callback);
  LOG(ERROR) << "maybe request queue is closed";

  // 4. 抓完清理
  // ------------------------------
  curl_multi_cleanup(curl_multi_handler_);

  close(epoll_fd_);

  //// XXX(suhua): 这个全局清理, 会导致该类不能多线程使用, 所以不能调用
  //// 在 析构函数中 调用
  //// curl_global_cleanup();
}
 */

// using select()
void Fetcher::FetchUrls(thread::BlockingQueue<UrlToFetch> *urls_to_fetch_queue,
                        FetchedUrlCallback *callback) {
  // 1. 初始化 curl multi handler
  // ------------------------------
  CHECK_NOTNULL(urls_to_fetch_queue);
  CHECK_EQ(thread_safe_curl_global_init(), CURLE_OK);

  curl_multi_handler_ = curl_multi_init();
  CHECK_NOTNULL(curl_multi_handler_);

  // 显示忽略掉 SIGPIP 信号，因为在 DNS 异步解析时可能会产生这个信号(即使设置了 CURLOPT_NOSIGNAL)
  CHECK_NE(signal(SIGPIPE, SIG_IGN), SIG_ERR);

  // 初始化代理状态
  proxy_stat_list_.clear();
  for (auto it = options_.proxy_servers.begin(); it != options_.proxy_servers.end(); ++it) {
    // XXX(suhua): 为每一个代理初始化一个空的状态, 这样后面 SelectBestProxy()
    // 里挑选代理时, 就可以直接 proxy_stat_list_, 而不需要遍历
    // options_.proxy_servers, 然后到 proxy_stat_list_ 里查表
    proxy_stat_list_[*it].proxy_server = *it;
  }

  // 3. 开始循环抓取, 不会退出, 直到队列关闭
  // ------------------------------
  LOG(ERROR) << "before loop in fetchurl";
  Loop(urls_to_fetch_queue, callback);
  LOG(ERROR) << "maybe request queue is closed";

  // 4. 抓完清理
  // ------------------------------
  curl_multi_cleanup(curl_multi_handler_);

  //// XXX(suhua): 这个全局清理, 会导致该类不能多线程使用, 所以不能调用
  //// 在 析构函数中 调用
  //// curl_global_cleanup();
}

// 每个线程的操作
/*
void Fetcher::FetchUrls(std::deque<UrlToFetch> *urls_to_fetch, FetchedUrlCallback *callback) {
  // 1. 初始化 curl multi handler
  // ------------------------------
  CHECK_NOTNULL(urls_to_fetch);
  CHECK_EQ(thread_safe_curl_global_init(), CURLE_OK);

  PLOG_IF(FATAL, (epoll_fd_ = epoll_create(kMaxEpollEvent)) == -1) << "failed to create epoll fd";
  VLOG(3) << "create epoll fd: " << epoll_fd_;

  curl_multi_handler_ = curl_multi_init();
  CHECK_NOTNULL(curl_multi_handler_);

  // 由 epoll fd 监听 multi handler 的事件
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETFUNCTION, CallbackSocket));
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETDATA, &epoll_fd_));

  // 通过 callback 获取 epoll 事件的超时时间
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERFUNCTION, &CallbackTimer));
  CHECK_EQ(CURLM_OK, curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERDATA, &epoll_timeout_ms_));

  // 显示忽略掉 SIGPIP 信号，因为在 DNS 异步解析时可能会产生这个信号(即使设置了 CURLOPT_NOSIGNAL)
  CHECK_NE(signal(SIGPIPE, SIG_IGN), SIG_ERR);

  // 初始化代理状态
  proxy_stat_list_.clear();
  for (auto it = options_.proxy_servers.begin(); it != options_.proxy_servers.end(); ++it) {
    // XXX(suhua): 为每一个代理初始化一个空的状态, 这样后面 SelectBestProxy()
    // 里挑选代理时, 就可以直接 proxy_stat_list_, 而不需要遍历
    // options_.proxy_servers, 然后到 proxy_stat_list_ 里查表
    proxy_stat_list_[*it].proxy_server = *it;
  }

  // 3. 开始循环抓取
  // ------------------------------
  LOG(ERROR) << "before loop in fetchurl, size: " << urls_to_fetch->size();
  Loop(urls_to_fetch, callback);
  LOG(ERROR) << "after loop in fetchurl";

  // 4. 抓完清理
  // ------------------------------
  curl_multi_cleanup(curl_multi_handler_);

  close(epoll_fd_);

  // XXX(suhua): 这个全局清理, 会导致该类不能多线程使用, 所以不能调用
  // 在 析构函数中 调用
  // curl_global_cleanup();
}
*/
int Fetcher::ObtainUrl(std::deque<UrlToFetch> *urls_to_fetch,
                       UrlToFetch *url_to_fetch) {
  if ((int)urls_to_fetch->size() > 0) {
    UrlToFetch &e = (*urls_to_fetch)[0];
    // 找到可抓 url
    *url_to_fetch = e;
    urls_to_fetch->pop_front();
    return 0;
  }
  return 1;
}

/*
// using epoll()
void Fetcher::Loop(thread::BlockingQueue<UrlToFetch> *urls_to_fetch_queue, FetchedUrlCallback *callback) {
  CHECK_NOTNULL(urls_to_fetch_queue);
  epoll_event events[kMaxEpollEvent];
  int32 msg_in_queue;  // 无用，丢弃

  int started_url_num = 0;
  int finished_url_num = 0;
  const int kIdleSlots = options_.concurrent_request_num; 
  LOG(ERROR) << "conn request num: " << kIdleSlots;
  int used_slots = 0;
  UrlToFetch url_to_fetch;
  while (true) {
    int status = urls_to_fetch_queue->TryTake(&url_to_fetch);
    // 如果队列为空 且 未 关闭, 则: 先查看 curl_multi_handler_ 是否有可以处理的消息; 然后
    // sleep(1) 并重新检测 blocking queue 是否有输入任务;
    if (status == 0) {
       if (finished_url_num != started_url_num) {
         DLOG(ERROR) << "finish:start is: " << finished_url_num << ":" << started_url_num;
         int running;
         curl_multi_socket_all(curl_multi_handler_, &running);
         while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
           LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
           FinishFetchUrl(msg, callback);
           --used_slots;
           ++finished_url_num;
         }
       }
       sleep(1);
      continue;
    }
    // 如果队列为空 且 关闭
    if (status == -1) {
       // 发起的 url 抓取请求和 完成的 url 请求数是相等的, 则退出循环, 抓取结束
       // 反之, 重复 check msg queue, 直到结束为止
       while (finished_url_num != started_url_num) {
         DLOG(ERROR) << "finish:start is: " << finished_url_num << ":" << started_url_num;
         int running;
         curl_multi_socket_all(curl_multi_handler_, &running);
         while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
           LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
           FinishFetchUrl(msg, callback);
           --used_slots;
           ++finished_url_num;
         }
         sleep(1);
       }
       break;
    }
    // 从队列中成功取出一个待抓取 url
    // 1. 尝试发起请求, 填满空闲的连接槽位
    while (used_slots == kIdleSlots) {
      DLOG(ERROR) << "used slots:idle slots: " << used_slots << ":" << kIdleSlots;
      sleep(1);
      // 处理抓取结果
      int running;
      curl_multi_socket_all(curl_multi_handler_, &running);
      while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
        LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
        FinishFetchUrl(msg, callback);
        --used_slots;
        ++finished_url_num;
      }
    }
    // 1. 发起抓取任务
    {
      StartFetchUrl(url_to_fetch);
      ++used_slots;
      ++started_url_num;
    }

    // 2. 触发 curl 的 callback, 并等待 epoll 事件
    {
      int running;
      curl_multi_socket_all(curl_multi_handler_, &running);
      VLOG(4) << "epoll wait, epoll fd: " << epoll_fd_
              << ", max_events: " << kMaxEpollEvent
              << ", epoll timeout: " << epoll_timeout_ms_ << "ms ";
      int status = epoll_wait(epoll_fd_, events, kMaxEpollEvent, epoll_timeout_ms_);
      PLOG_IF(FATAL, status == -1 && errno != EINTR) << "epoll_wait() fail";
      // 处理抓取结果
      while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
        LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
        FinishFetchUrl(msg, callback);
        --used_slots;
        ++finished_url_num;
      }
    }
  }
}

 */

void Fetcher::Loop(thread::BlockingQueue<UrlToFetch> *urls_to_fetch_queue, FetchedUrlCallback *callback) {
  CHECK_NOTNULL(urls_to_fetch_queue);
  int32 msg_in_queue;  // 无用，丢弃

  int started_url_num = 0;
  int finished_url_num = 0;
  const int kIdleSlots = options_.concurrent_request_num; 
  LOG(ERROR) << "conn request num: " << kIdleSlots;
  int used_slots = 0;
  UrlToFetch url_to_fetch;

  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  int maxfd = -1;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 10000;
  long curl_timeo = -1;
  while (true) {
    int status = urls_to_fetch_queue->TryTake(&url_to_fetch);
    // 如果队列为空 且 未 关闭, 则: 先查看 curl_multi_handler_ 是否有可以处理的消息; 然后
    // sleep(1) 并重新检测 blocking queue 是否有输入任务;
    if (status == 0) {
       if (finished_url_num != started_url_num) {
         DLOG(ERROR) << "finish:start is: " << finished_url_num << ":" << started_url_num;
         curl_multi_timeout(curl_multi_handler_, &curl_timeo);
         if(curl_timeo >= 0) {
           timeout.tv_sec = curl_timeo / 1000;
           if(timeout.tv_sec > 1)
             timeout.tv_sec = 1;
           else
             timeout.tv_usec = (curl_timeo % 1000) * 1000;
         }
         int running;
         FD_ZERO(&fdread);
         FD_ZERO(&fdwrite);
         FD_ZERO(&fdexcep);
         curl_multi_fdset(curl_multi_handler_, &fdread, &fdwrite, &fdexcep, &maxfd);
         int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
         LOG_IF(FATAL, rc == -1) << "select() fail";
         curl_multi_perform(curl_multi_handler_, &running);
         while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
           LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
           FinishFetchUrl(msg, callback);
           --used_slots;
           ++finished_url_num;
         }
       }
      continue;
    }
    // 如果队列为空 且 关闭
    if (status == -1) {
       // 发起的 url 抓取请求和 完成的 url 请求数是相等的, 则退出循环, 抓取结束
       // 反之, 重复 check msg queue, 直到结束为止
       while (finished_url_num != started_url_num) {
         DLOG(ERROR) << "finish:start is: " << finished_url_num << ":" << started_url_num;
         curl_multi_timeout(curl_multi_handler_, &curl_timeo);
         if(curl_timeo >= 0) {
           timeout.tv_sec = curl_timeo / 1000;
           if(timeout.tv_sec > 1)
             timeout.tv_sec = 1;
           else
             timeout.tv_usec = (curl_timeo % 1000) * 1000;
         }
         int running;
         FD_ZERO(&fdread);
         FD_ZERO(&fdwrite);
         FD_ZERO(&fdexcep);
         curl_multi_fdset(curl_multi_handler_, &fdread, &fdwrite, &fdexcep, &maxfd);
         int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
         LOG_IF(FATAL, rc == -1) << "select() fail";
         curl_multi_perform(curl_multi_handler_, &running);
         while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
           LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
           FinishFetchUrl(msg, callback);
           --used_slots;
           ++finished_url_num;
         }
       }
       break;
    }
    // 从队列中成功取出一个待抓取 url
    // 1. 尝试发起请求, 填满空闲的连接槽位
    while (used_slots == kIdleSlots) {
      DLOG(ERROR) << "used slots:idle slots: " << used_slots << ":" << kIdleSlots;
      // 处理抓取结果
      curl_multi_timeout(curl_multi_handler_, &curl_timeo);
      if(curl_timeo >= 0) {
        timeout.tv_sec = curl_timeo / 1000;
        if(timeout.tv_sec > 1)
          timeout.tv_sec = 1;
        else
          timeout.tv_usec = (curl_timeo % 1000) * 1000;
      }
      int running;
      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);
      curl_multi_fdset(curl_multi_handler_, &fdread, &fdwrite, &fdexcep, &maxfd);
      int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
      LOG_IF(FATAL, rc == -1) << "select() fail";
      curl_multi_perform(curl_multi_handler_, &running);
      while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
        LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
        FinishFetchUrl(msg, callback);
        --used_slots;
        ++finished_url_num;
      }
    }
    // 1. 发起抓取任务
    {
      StartFetchUrl(url_to_fetch);
      ++used_slots;
      ++started_url_num;
    }

    // 2. 触发 curl 的 callback, 并接收 & 处理消息
    {
      curl_multi_timeout(curl_multi_handler_, &curl_timeo);
      if(curl_timeo >= 0) {
        timeout.tv_sec = curl_timeo / 1000;
        if(timeout.tv_sec > 1)
          timeout.tv_sec = 1;
        else
          timeout.tv_usec = (curl_timeo % 1000) * 1000;
      }
      int running;
      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);
      curl_multi_fdset(curl_multi_handler_, &fdread, &fdwrite, &fdexcep, &maxfd);
      int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
      LOG_IF(FATAL, rc == -1) << "select() fail";
      curl_multi_perform(curl_multi_handler_, &running);
      // 处理抓取结果
      while (CURLMsg *msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue)) {
        LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
        FinishFetchUrl(msg, callback);
        --used_slots;
        ++finished_url_num;
      }
    }
  }
}
/*
void Fetcher::Loop(std::deque<UrlToFetch> *urls_to_fetch, FetchedUrlCallback *callback) {
  CHECK_NOTNULL(urls_to_fetch);
  epoll_event events[kMaxEpollEvent];

  int started_url_num = 0;
  int finished_url_num = 0;
  int total_url_num = (int)urls_to_fetch->size();
  const int kIdleSlots = 1200;
  int used_slots = 0;
  while (total_url_num > finished_url_num) {
    // 1. 尝试发起请求, 填满空闲的连接槽位
    if (used_slots == kIdleSlots) {
      LOG(ERROR) << "used_slots == kIdleSlots, will sleep(3)";
      sleep(3);
      --used_slots;
    }
    while (used_slots < kIdleSlots) {
      UrlToFetch url_to_fetch;
      int ret = ObtainUrl(urls_to_fetch, &url_to_fetch);
      // 待抓取列表没有新任务
      if (ret == 0) {
        ++used_slots;
        ++started_url_num;
        // 发起抓取请求
        //auto it = options_.client_redirect_urls.find(url_to_fetch.url);
        //if (it != options_.client_redirect_urls.end()) {
        //  url_to_fetch.client_redirect_url = it->second;
        //} else {
        //  url_to_fetch.client_redirect_url.clear();
        //}
        StartFetchUrl(url_to_fetch);
      }

      // 2. 触发 curl 的 callback, 并等待 epoll 事件
      int running;
      curl_multi_socket_all(curl_multi_handler_, &running);

      VLOG(4) << "epoll wait, epoll fd: " << epoll_fd_
              << ", max_events: " << kMaxEpollEvent
              << ", epoll timeout: " << epoll_timeout_ms_ << "ms ";
      int status = epoll_wait(epoll_fd_, events, kMaxEpollEvent, epoll_timeout_ms_);
      PLOG_IF(FATAL, status == -1 && errno != EINTR) << "epoll_wait() fail";

      // 3. 处理抓取结果
      CURLMsg *msg;
      int32 msg_in_queue;  // 无用，丢弃
      while ((msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue))) {
        LOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
        --used_slots;
        ++finished_url_num;
        FinishFetchUrl(msg, callback);
      }
      if (finished_url_num == total_url_num) break;
    }
  }
}
*/
void Fetcher::StartFetchUrl(const UrlToFetch &url_to_fetch) {
  // 1. 初始化 easy handler
  CURL *eh = curl_easy_init();
  CHECK_NOTNULL(eh);
  SetEasyHandler(eh, url_to_fetch);

  // 2. 加入到 multi handler
  curl_multi_add_handle(curl_multi_handler_, eh);

  // 3. 修改 proxy 状态
  if (url_to_fetch.use_proxy) {
    const PrivateData *private_data;
    LOG_IF(FATAL, CURLE_OK != curl_easy_getinfo(eh, CURLINFO_PRIVATE, &private_data));
    ++proxy_stat_list_[private_data->proxy_server_used].started;
    ++proxy_stat_list_[private_data->proxy_server_used].connections;
  }
}

void Fetcher::FinishFetchUrl(CURLMsg *msg, FetchedUrlCallback *callback) {
  // 取出 private data
  PrivateData *private_data;
  LOG_IF(FATAL, CURLE_OK != curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &private_data))
      << "Get private data curl_easy_getinfo() fail.";
  // 函数退出时自动删除 private data
  scoped_ptr<PrivateData> auto_delete(private_data);

  // 1. 生成 url fetched (包括它的成员 proto Resource)
  // |url_fetched| 会被发送给外部 callback，且由外部 callback 管理生命周期
  UrlFetched *url_fetched = new UrlFetched();
  CHECK(ExtractInfoFromCurl(msg, private_data, url_fetched));

  DVLOG(10) << "crawl url(" << url_fetched->url_to_fetch.url << ") "
            << url_fetched->success << " with retcode: "
            << url_fetched->ret_code;

  // 修改 proxy 状态
  if (private_data->url_to_fetch.use_proxy) {
    ProxyStat &proxy_stat = proxy_stat_list_[private_data->proxy_server_used];

    ++proxy_stat.finished;
    --proxy_stat.connections;

    if (url_fetched->success) {
      proxy_stat.successive_failures = 0;
    } else {
      proxy_stat.successive_failures += 1;

      if (proxy_stat.successive_failures == options_.proxy_max_successive_failures) {
        LOG(ERROR) << "found a proxy failure: " << proxy_stat.proxy_server;
      }
    }
  }

  // 3. 删除 easy handler
  CURL *eh = msg->easy_handle;
  // XXX(suhua): once curl_multi_remove_handle() is called, |msg| is invalid
  // and can't be used.
  curl_multi_remove_handle(curl_multi_handler_, eh);
  // XXX(suhua): use |eh| init before, should not use msg->easy_handle, for
  // |msg| is invalid after the curl call above.
  curl_easy_cleanup(eh);

  // 4. 调用应用的 callback
  // 由应用负责销毁 url_fetched;
  callback->Process(url_fetched);
}

void Fetcher::DiscardFetchUrl(const UrlToFetch &url_to_fetch, FetchedUrlCallback *callback) {
  UrlFetched *url_fetched = new UrlFetched();

  url_fetched->url_to_fetch = url_to_fetch;
  url_fetched->start_time = base::GetTimestamp();
  url_fetched->end_time = base::GetTimestamp();
  url_fetched->success = false;
  url_fetched->ret_code = -1;
  url_fetched->reason = "discarded due to too many failures on ip " + url_to_fetch.ip;

  callback->Process(url_fetched);
}

const std::string &Fetcher::SelectBestProxy() const {
  // XXX(suhua): 每次挑选 当前连接数最少 的代理; 这样, 速度快的代理会被充分利用
  // 少数慢代理也不会拖慢整体性能
  int64 min_connections = kInt64Max;
  const ProxyStat *best_proxy = NULL;
  int invalid_proxy_num = 0;
  for (auto it = proxy_stat_list_.begin(); it != proxy_stat_list_.end(); ++it) {
    if (it->second.successive_failures >= options_.proxy_max_successive_failures) {
      ++invalid_proxy_num;
      continue;
    }
    if (it->second.connections < min_connections) {
      best_proxy = &it->second;
      min_connections = it->second.connections;
    }
  }
  CHECK_NOTNULL(best_proxy);

  if (invalid_proxy_num > (int)proxy_stat_list_.size() / 2) {
    // 超过一半的代理挂了, 打印错误信息, 并崩溃掉, 此时需要人工介入
    for (auto it = proxy_stat_list_.begin(); it != proxy_stat_list_.end(); ++it) {
      if (it->second.successive_failures >= options_.proxy_max_successive_failures) {
        LOG(ERROR) << "proxy failure: " << it->second.proxy_server;
      }
    }

    LOG(FATAL) << "挂掉太多代理了, 请检查代理设置";
  }

  return best_proxy->proxy_server;
}

}  // namespace crawler2

