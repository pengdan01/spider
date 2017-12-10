#pragma once

#include <string>
#include <map>

#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/thread/blocking_queue.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"

#include "crawler/crawl/time_split_saver.h"
#include "crawler/realtime_crawler/extra.h"
#include "crawler/realtime_crawler/job_manager.h"
#include "crawler/realtime_crawler/realtime_crawler.h"

namespace crawler2 {
namespace crawl_queue {
class SubmitCrawlResultHandler : public CrawlTaskHandler {
 public:
  struct Options {
    std::string status_prefix;
    int64 saver_timespan;
    std::string default_queue_ip;
    int32 default_queue_port;
    std::string default_queue_tube;
    Options()
        : saver_timespan(3600l * 1000 * 1000)
        , default_queue_port(0) {
    }
  };

  explicit SubmitCrawlResultHandler(const Options& options);
  ~SubmitCrawlResultHandler();

  void Init();
  virtual void Process(crawl::CrawlTask* task,
                       crawl::CrawlRoutineStatus* status,
                       Resource* res, Closure* done);
  void Stop();
  int ResourceQueueSize() const { return queue_.Size();}
 private:
  void SubmitJob(const JobOutput& output);
  void SubmitProc();
  ::thread::BlockingQueue<JobOutput*> queue_;

  std::string status_file_prefix;
  crawler2::crawl::TimeSplitSaver status_saver_;
  Options options_;

  std::atomic<bool> stopped_;
  ::thread::Thread thread_;
  int total_processed_;
  std::map<std::string, job_queue::Client*> clients_;
};
}  // namespace crawl_queue
}  // namespace crawler2
