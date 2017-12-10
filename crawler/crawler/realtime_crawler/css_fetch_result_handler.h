#pragma once

#include <string>
#include <map>

#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/thread/blocking_queue.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"

#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "rpc/job_queue/job_queue.h"

namespace crawler2 {
namespace crawl_queue {
class SubmitResWithCssHandler {
 public:
  struct Options {
    std::string default_queue_ip;
    int32 default_queue_port;
    std::string default_queue_tube;
    Options()
        : default_queue_port(0) {
    }
  };

  explicit SubmitResWithCssHandler(const Options& options);
  ~SubmitResWithCssHandler();

  void Init();
  void Stop();
  int ResourceQueueSize() const { return queue_.Size();}

  void SubmitJob(const JobOutput& output);
 private:
  void SubmitProc();
  ::thread::BlockingQueue<JobOutput*> queue_;

  Options options_;

  std::atomic<bool> stopped_;
  ::thread::Thread thread_;
  int total_processed_;
  std::map<std::string, job_queue::Client*> clients_;
};
}  // namespace crawl_queue
}  // namespace crawler2
