#pragma once
#include <cstdatomic>

#include <string>
#include <vector>

#include "base/thread/thread.h"
#include "base/thread/thread_pool.h"
#include "base/thread/thread_util.h"
#include "base/common/closure.h"
#include "extend/config/config.h"
#include "feature/web/page/html_features.pb.h"
#include "crawler/crawl/time_split_saver.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler/realtime_crawler/index_request_submitter.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"

#include "web/url_norm/url_norm.h"
#include "web/url_category/url_categorizer.h"

namespace redis {
class RedisController;
}
namespace crawler2 {
namespace crawl_queue {
class RealtimeExtractor {
 public:
  struct Options {
    std::string resource_prefix;
    std::string title_prefix;
    int64 saver_timespan;  // 微秒
    int64 res_saver_timespan;  // 微秒
    int processor_thread_num;  // 分析网页的线程数量

    std::string job_queue_ip;
    int job_queue_port;
    std::string job_queue_tube;
    std::string error_titles;

    Options()
        : job_queue_port(0) {
    }
  };

  RealtimeExtractor(const Options& options,
                    IndexRequestSubmitter* index_request_submitter,
                    IndexRequestSubmitter* index_request_submitter2,
                    redis::RedisController& redis_controler,
                    bool mode);
  ~RealtimeExtractor() {}

  void Start();
  void Stop();
  void Loop();

  int ResourceQueueSize();
 private:
  struct ArchiveData {
    std::string title;
    std::string cdoc;
    JobOutput *job;

    ArchiveData() : job(NULL) {
    }
  };
  // 此线程负责分析网页
  void ExtractorProc();

  // 此线程负责不断从任务队列取任务
  void FetchJobProc();

  // 此线程负责向 realtime_indexer 提交任务
  void SubmitProc();

  void SubmitDone();

  // 此函数负责将 res 转换成和大爬虫相同的 存储格式
  void SaveResourceInKeyValueFormat(const crawler2::Resource &res);   

  // 此函数负责给查询 redis 内存词典, 补全 res 如下信息: uv rank, query, anchor 
  bool AddResourceExtraInfo(crawler2::Resource &res, std::string *desc);

  std::vector< ::thread::Thread*> processor_threads_;
  ::thread::Thread fetch_job_thread_;
  ::thread::Thread submit_thread_;
  ::thread::Thread title_thread_;
  ::thread::BlockingQueue<JobOutput*> resources_;
  ::thread::BlockingQueue<ArchiveData> archives_;
  Options options_;
  bool stopped_;

  IndexRequestSubmitter* index_request_submitter_;
  IndexRequestSubmitter* index_request_submitter2_;

  crawl::TimeSplitSaver resource_saver_;
  crawl::TimeSplitSaver title_saver_;

 public:
  // 统计数据
  std::atomic<int64> total_fetched_;  // 从任务队列当中取出的数量
  std::atomic<int64> total_cdocs_generated_;   // 产生的数量
  std::atomic<int64> total_no_res_;   // 取到的任务当中不包含 res 的数量
  std::atomic<int64> total_failed_;   // 生成失败的数量
 private:
  ::web::UrlNorm url_norm_;
  ::redis::RedisController& redis_controler_;
  bool backup_mode_;
  web::util::UrlCategorizer url_categoryizer_;
};

void ParseRealtimeExtractorConfig(const extend::config::ConfigObject& config,
                                  RealtimeExtractor::Options* options);
}  // namespace crawl_queue
}  // namespace crawler2
