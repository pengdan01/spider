#pragma once

#include <cstdatomic>
#include <deque>

#include "base/thread/thread.h"
#include "base/common/closure.h"
#include "base/common/logging.h"
#include "crawler/realtime_crawler/job_manager.h"
#include "crawler/realtime_crawler/realtime_crawler.h"

namespace crawler2 {
namespace crawl_queue {

class JobMonitor {
 public:
  JobMonitor(JobManager* manager, RealtimeCrawler* crawler)
      : manager_(manager)
      , crawler_(crawler)
      , stop_(false) {
    CHECK_NOTNULL(manager);
    CHECK_NOTNULL(crawler);
  }

  void Start() {
    thread_.Start(NewCallback(this, &JobMonitor::SupplyTask));
  }

  void Stop() {
    stop_ = true;
    thread_.Join();
  }
 private:
  void SupplyTask() {
    thread::SetCurrentThreadName("JobMonitor[SupplyTask]");
    while (!stop_) {
      manager_->UpdateFetchNum(crawler_->GetQueueIdelNum());
      if (crawler_->GetQueueIdelNum() <= 0) {
        LOG_EVERY_N(INFO, 10000) << "No free slot["
                                 << crawler_->GetIdleSlotNum() <<"]";
        usleep(50 * 1000);
      }

      std::deque<RealtimeCrawler::JobData*> deq;
      manager_->TakeAll(&deq);

      while (!deq.empty()) {
        crawler_->AddTask(deq.front());
        VLOG(5) << "added to crawl queue, url: " << deq.front()->job.detail().url(); 
        deq.pop_front();
      }
    }
  }

  ::thread::Thread thread_;
  JobManager* manager_;
  RealtimeCrawler* crawler_;
  std::atomic<bool> stop_;
};
}  // namespace crawl_queue
}  // namespace crawler2
