#pragma once

#include <cstdatomic>
#include <deque>

#include "base/common/sleep.h"
#include "base/thread/thread.h"
#include "base/thread/thread_util.h"
#include "base/common/closure.h"
#include "base/common/logging.h"

#include "spider/crawler/job_manager.h"
#include "spider/crawler/crawler.h"

namespace spider{

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
    while (manager_->GetPendingJobCnt() > 0) {
      LOG(INFO) << "wait the jobs_to_put_ queue to empty, current size is " << manager_->GetPendingJobCnt();
      base::SleepForSeconds(1);
    }
    stop_ = true;
    LOG(INFO) << "[Stop] start to wait job_monitor exit";
    thread_.Join();
    LOG(INFO) << "[Stop] job_monitor exit successfully";
  }
 private:
  void SupplyTask() {
    thread::SetCurrentThreadName("JobMonitor[SupplyTask]");
    while (!stop_) {
      int32 idle_slot = crawler_->GetQueueIdelNum();
      if (idle_slot <= 0) {
        LOG_EVERY_N(INFO, 10000) << "No free slot[" << idle_slot <<"]";
        usleep(1 * 1000);
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
}  // namespace spider 
