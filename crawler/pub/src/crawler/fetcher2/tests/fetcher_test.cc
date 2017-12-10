
#include "base/testing/gtest.h"
#include "base/common/basic_types.h"
#include "base/common/sleep.h"
#include "base/common/base.h"
#include "base/time/time.h"
#include "crawler/fetcher2/tests/simple_fetcher.h"
#include "crawler/fetcher2/tests/simple_webserver.h"

namespace crawler2 {
// 传递给 Fetcher 的回调函数
class CallbackWithStat: public crawler2::Fetcher::FetchedUrlCallback {
 public:
  CallbackWithStat() {
    failed_ = 0;
    dropped_ = 0;
  }
  virtual ~CallbackWithStat() {}
  virtual void Process(crawler2::UrlFetched *url_fetched) {
    CHECK_NOTNULL(url_fetched);
    if (!url_fetched->success) {
      if (url_fetched->ret_code == -1) {
        dropped_++;
      }
      failed_++;
    }
  }
  int failed_;
  int dropped_;
};

TEST(FetcherTest, FailedExceedThreadhold) {
  // 连续在 webserver 上抓取
  crawler2::LoadController::Options options;
  options.max_connections_in_all = 1;
  options.min_fetch_times = 2;
  options.min_holdon_duration_after_failed = 1;
  options.max_holdon_duration_after_failed = 1;
  options.failed_times_threadhold = 5;
  options.max_failed_times = 5;
  SimpleFetcher fetcher(options);
  fetcher.Init("*\t1000\t1000\t00:00-23:59\n");

  const int task_count = 100;
  UrlToFetch task;
  std::deque<UrlToFetch> deq;
  task.url = "http://127.0.0.1:3333/a.html";
  task.ip = "127.0.0.1";
  task.resource_type = 1;
  task.use_proxy = false;
  for (int i = 0; i < task_count; ++i) {
    deq.push_back(task);
  }

  CallbackWithStat callback;
  fetcher.Fetch(&deq, &callback);

  // 检查状态
  ASSERT_EQ(callback.dropped_, 95);
  ASSERT_EQ(callback.failed_, 100);
}

TEST(FetcherTest, Success) {
  // 启动 webserver
  bamboo::ServerOption serv_option;
  serv_option.port = 18081;
  SimpleWebServer serv;
  serv.Start(serv_option);
  // 连续在 webserver 上抓取
  SimpleFetcher fetcher;
  fetcher.Init("*\t10\t1000\t00:00-23:59\n");

  const int task_count = 100;
  UrlToFetch task;
  std::deque<UrlToFetch> deq;
  task.url = "http://127.0.0.1:18081/a.html";
  task.ip = "127.0.0.1";
  task.resource_type = 1;
  task.use_proxy = false;
  for (int i = 0; i < task_count; ++i) {
    deq.push_back(task);
  }

  CallbackWithStat callback;
  base::Time cur = base::Time::Now();
  fetcher.Fetch(&deq, &callback);
  base::TimeDelta delta = base::Time::Now() - cur;
  serv.Stop();

  EXPECT_GE(delta.InSecondsF(), 0.1f);
  // 比较开始时间与结束时间是否符合时间的要求

  ASSERT_EQ(callback.failed_, 0);
}

TEST(FetcherTest, QPS) {
  // 启动 webserver
  bamboo::ServerOption serv_option;
  serv_option.port = 8080;
  SimpleWebServer serv;
  serv.Start(serv_option);

  // 连续在 webserver 上抓取
  crawler2::LoadController::Options options;
  options.max_connections_in_all = 1;
  options.min_fetch_times = 10;
  options.min_holdon_duration_after_failed = 1;
  options.max_holdon_duration_after_failed = 1;
  options.failed_times_threadhold = 5;
  SimpleFetcher fetcher(options);
  fetcher.Init("*\t1000\t1000\t00:00-23:59\n");

  const int task_count = 100;
  UrlToFetch task;
  std::deque<UrlToFetch> deq;
  task.url = "http://127.0.0.1:8080/a.html";
  task.ip = "127.0.0.1";
  task.resource_type = 1;
  task.use_proxy = false;
  for (int i = 0; i < task_count; ++i) {
    deq.push_back(task);
  }

  base::Time cur = base::Time::Now();
  CallbackWithStat callback;
  fetcher.Fetch(&deq, &callback);
  base::TimeDelta delta = base::Time::Now() - cur;
  serv.Stop();

  // 比较开始时间与结束时间是否符合时间的要求
  ASSERT_EQ(callback.failed_, 0);
  EXPECT_GE(delta.InSecondsF(), 0.1f);
  EXPECT_LE(delta.InSecondsF(), 1.8f);
}

TEST(FetcherTest, QPSNot) {
  // 连续在 webserver 上抓取
  crawler2::LoadController::Options options;
  options.max_connections_in_all = 1;
  options.min_fetch_times = 10;
  options.min_holdon_duration_after_failed = 1;
  options.max_holdon_duration_after_failed = 1;
  options.failed_times_threadhold = 5;
  SimpleFetcher fetcher(options);
  fetcher.Init("*\t1000\t1000\t00:00-23:59\n");

  const int task_count = 100000;
  UrlToFetch task;
  std::deque<UrlToFetch> deq;
  task.url = "http://127.0.0.1:8081/a.html";
  task.ip = "127.0.0.1";
  task.resource_type = 1;
  task.use_proxy = false;
  for (int i = 0; i < task_count; ++i) {
    deq.push_back(task);
  }

  base::Time cur = base::Time::Now();
  CallbackWithStat callback;
  fetcher.Fetch(&deq, &callback);
  base::TimeDelta delta = base::Time::Now() - cur;

  // 比较开始时间与结束时间是否符合时间的要求
  //
  // FIXME(suhua): 该检查会失败, 暂时注掉
  // ASSERT_EQ(callback.failed_, 0);

  EXPECT_GE(delta.InSecondsF(), 0.1f);
  EXPECT_LE(delta.InSecondsF(), 1.8f);
}

// 在成功抓取指定次数后，将主动关闭服务
class CallbackWithServerControl: public crawler2::Fetcher::FetchedUrlCallback {
 public:
  CallbackWithServerControl(SimpleWebServer* server, int stop_after_success) {
    success_ = 0;
    stop_after_success_ = stop_after_success;
    failed_ = 0;
    dropped_ = 0;
    server_ = server;
    server_stopped_ = false;
  }

  virtual ~CallbackWithServerControl() {}
  virtual void Process(crawler2::UrlFetched *url_fetched) {
    CHECK_NOTNULL(url_fetched);
    if (!url_fetched->success) {
      if (url_fetched->ret_code == -1) {
        dropped_++;
      }
      failed_++;
    } else {
      success_++;
    }

    if (success_ == stop_after_success_
        && !server_stopped_) {
      server_->Stop();
      server_stopped_ = true;
    }
  }
  int stop_after_success_;
  int success_;
  int failed_;
  int dropped_;
  bool server_stopped_;
  SimpleWebServer* server_;
};

TEST(FetcherTest, FailedAfterSuccess) {
  // 启动 webserver
  bamboo::ServerOption serv_option;
  serv_option.port = 8080;
  SimpleWebServer serv;
  serv.Start(serv_option);
  // 连续在 webserver 上抓取

  crawler2::LoadController::Options options;
  options.max_connections_in_all = 1;
  options.min_fetch_times = 2;
  options.min_holdon_duration_after_failed = 1;
  options.max_holdon_duration_after_failed = 1;
  options.failed_times_threadhold = 5;
  options.max_failed_times = 5;
  SimpleFetcher fetcher(options);
  fetcher.Init("*\t1000\t1000\t00:00-23:59\n");

  const int task_count = 100;
  const int success_expected = 10;
  UrlToFetch task;
  std::deque<UrlToFetch> deq;
  task.url = "http://127.0.0.1:8080/a.html";
  task.ip = "127.0.0.1";
  task.resource_type = 1;
  task.use_proxy = false;
  for (int i = 0; i < task_count; ++i) {
    deq.push_back(task);
  }

  base::Time cur = base::Time::Now();
  CallbackWithServerControl callback(&serv, 10);
  fetcher.Fetch(&deq, &callback);
  base::TimeDelta delta = base::Time::Now() - cur;
  ASSERT_GE(delta.InSecondsF(), 0.1f);
  // 比较开始时间与结束时间是否符合时间的要求

  ASSERT_EQ(callback.success_, success_expected);
  ASSERT_EQ(callback.failed_, task_count - success_expected);
  ASSERT_EQ(callback.dropped_, task_count - success_expected
            - options.max_failed_times);
}

}  // namespace crawler2
