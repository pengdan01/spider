#pragma once

#include "base/common/closure.h"
#include "crawler/realtime_crawler/realtime_crawler.h"

class CounterCrawlTaskHandler : public crawler2::crawl_queue::CrawlTaskHandler {
 public:
  CounterCrawlTaskHandler() : count(0) {}
  void Process(crawler2::crawl::CrawlTask* task,
               crawler2::crawl::CrawlRoutineStatus* status,
               crawler2::Resource* res,  Closure* done) {
    ScopedClosure atuo_closure(done);
    count++;
  }

  std::atomic<int> count;
};
