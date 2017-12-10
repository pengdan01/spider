#pragma once

#include <string>
#include <map>
#include <vector>

#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/thread/blocking_queue.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"

#include "crawler/crawl/time_split_saver.h"
#include "spider/crawler/job_manager.h"
#include "spider/crawler/crawler.h"
#include "spider/crawler/kafka/kafka_writer.h"

namespace spider {

using crawler2::crawl_queue::JobInput;
using crawler2::crawl_queue::JobOutput;
using crawler2::crawl_queue::JobDetail;

class SubmitCrawlResultHandler : public CrawlTaskHandler {
 public:
  struct Options {
    std::string status_prefix;
    int64 saver_timespan;
    // 正常发给下游的 job queue 配置
    std::string kafka_brokers;
    std::string kafka_job_result_topic;

    // url 抓取写回的 job queue 配置
    std::string kafka_job_request_topic;
    // 最多尝试次数, 如果超过，job 将被丢弃
    int max_retry_times;

    Options()
        : saver_timespan(1200 * 1000 * 1000), max_retry_times(2) {
    }
  };

  explicit SubmitCrawlResultHandler(const Options &options);
  virtual ~SubmitCrawlResultHandler();

  void Init();
  virtual void Process(crawler2::crawl::CrawlTask *task,
                       crawler2::crawl::CrawlRoutineStatus *status,
                       crawler2::Resource *res, Closure *done);
  void Stop();
  int ResourceQueueSize() const { return queue_.Size(); }
 private:
  void SubmitJob(const JobOutput &output);
  void SubmitProc();
  bool ConvertHtmlToUtf8(JobOutput *output);

  bool IsNeedRetry(JobOutput *output);

  // 献策负责将需要重试抓取的 url 放回请求的kafka队列，方便以后再重新调度
  void PutJobBackProc(JobInput *job, int32_t from_partition);

  thread::BlockingQueue<JobOutput *> queue_;

  std::string status_file_prefix;
  crawler2::crawl::TimeSplitSaver status_saver_;
  Options options_;

  std::atomic<bool> stopped_;
  thread::ThreadPool *submit_threads_;
  std::atomic<int64> total_send_;

  KafkaWriter *kafka_writer_;
};
}  // namespace spider 
