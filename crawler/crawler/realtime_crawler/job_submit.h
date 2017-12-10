#pragma once

#include <deque>
#include <string>
#include "base/thread/sync.h"
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "rpc/job_queue/job_queue.h"

namespace crawler2 {
namespace crawl_queue {

DECLARE_int32(job_default_priority);

class CrawlJobSubmitter {
 public:
  struct Option {
    // 服务的 ip 及 端口
    std::string queue_ip;
    int queue_port;
    std::string queue_tube;
    Option() : queue_port(20080) {
    }
  };

  explicit CrawlJobSubmitter(const Option& option)
      : option_(option)
      , job_fetcher_(option.queue_ip, option.queue_port)
      , started_(false) {
    CHECK(!option.queue_ip.empty());
  }

  ~CrawlJobSubmitter() {
    Stop();
  }

  bool Init();
  bool AddJob(const JobInput& item);
  bool AddJob(const JobDetail& detail);
  bool Ping() {
    return job_fetcher_.Ping();
  }
 private:
  bool ConnectDetact();
  void Stop();
  void SubmitProc();
  Option option_;
  job_queue::Client job_fetcher_;
  std::atomic<bool> started_;
  ::thread::Thread thread;
  ::thread::BlockingQueue<JobInput*> jobs_;
};
}  // namespace crawl_queue
}  // namespace crawler2
