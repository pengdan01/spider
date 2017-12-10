#pragma once

#include <string>
#include <map>
#include <vector>

#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/thread/blocking_queue.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"
#include "feature/redis_dict/redis_dict_util.h"

#include "crawler/crawl/time_split_saver.h"
#include "crawler2/general_crawler/job_manager.h"
#include "crawler2/general_crawler/crawler.h"
#include "crawler2/general_crawler/url.pb.h"

namespace crawler3 {
DECLARE_bool(report_crawl_status_to_redis);
DECLARE_string(redis_servers_list);

using crawler2::crawl_queue::JobInput;
using crawler2::crawl_queue::JobOutput;
using crawler2::crawl_queue::JobDetail;

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
  virtual ~SubmitCrawlResultHandler();

  void Init();
  virtual void Process(crawler2::crawl::CrawlTask* task,
                       crawler2::crawl::CrawlRoutineStatus* status,
                       crawler2::Resource* res, Closure* done);
  void Stop();
  int ResourceQueueSize() const { return queue_.Size();}
 private:
  void SubmitJob(const JobOutput& output);
  void SubmitStatus(const url::CrawledUrlStatus& status);

  void SubmitProc();
  void ReportStatusProc();

  inline int32 GetRedisServerId(uint64 url_sign) {
    int32 id = url_sign % (int32)redis_clients_.size();
    return id;
  }

  thread::BlockingQueue<JobOutput*> queue_;

  std::string status_file_prefix;
  crawler2::crawl::TimeSplitSaver status_saver_;
  Options options_;

  std::atomic<bool> stopped_;
  thread::Thread thread_;
  int total_processed_;
  std::map<std::string, job_queue::Client*> clients_;

  thread::Thread report_status_thread_;
  std::vector<std::pair<std::string, int> > server_ip_;
  thread::BlockingQueue<url::CrawledUrlStatus*> status_queue_;
  std::map<int32, redis::Client*> redis_clients_;
};
}  // namespace crawler3
