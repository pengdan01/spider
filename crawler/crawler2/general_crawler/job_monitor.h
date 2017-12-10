#pragma once

#include <cstdatomic>
#include <deque>

#include "base/thread/thread.h"
#include "base/common/closure.h"
#include "base/common/logging.h"

#include "crawler2/general_crawler/job_manager.h"
#include "crawler2/general_crawler/crawler.h"

namespace crawler3 {

class JobMonitor {
 public:
  JobMonitor(JobManager* manager, GeneralCrawler* crawler)
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
      int32 idle_slot = crawler_->GetQueueIdelNum();
      if (idle_slot <= 0) {
        LOG_EVERY_N(INFO, 10000) << "No free slot[" << idle_slot <<"]";
        usleep(30 * 1000);
        continue;
      }

      std::deque<GeneralCrawler::JobData*> deq;
      manager_->TakeAll(&deq);

      while (!deq.empty()) {
        crawler_->AddTask(deq.front());
        VLOG(5) << "added to crawl queue, url: " << deq.front()->job.detail().url();
        deq.pop_front();
      }
      manager_->UpdateFetchNum(idle_slot);
    }
  }

  thread::Thread thread_;
  JobManager* manager_;
  GeneralCrawler* crawler_;
  std::atomic<bool> stop_;
};
}  // namespace crawler3
