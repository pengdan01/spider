#pragma once
#include <cstdatomic>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

#include "base/common/gflags.h"
#include "base/thread/thread.h"
#include "base/thread/sync.h"
#include "base/common/closure.h"
#include "base/hash_function/url.h"
#include "base/random/pseudo_random.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/job_queue/job_queue.h"
#include "web/url_category/url_categorizer.h"

#include "crawler2/general_crawler/redis_const.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler2/general_crawler/url.pb.h"

namespace crawler3 {

DECLARE_int32(thread_num);
DECLARE_string(index_model_bin_file_path);
DECLARE_double(index_model_score_threshold);

class Scheduler {
 public:
  struct Options {
    // Job queue 相关属性
    std::string queue_ip;
    int32 queue_port;
    std::string queue_tube;
  };
  // 一个输入队列, 若干个输出队列.
  // 根据调度策略决定分发到某一个输出队列.
  explicit Scheduler(const Options &in_options, const std::vector<Options> &out_options,
                     redis::RedisController &controler);
  ~Scheduler();
  bool Init();
  void Start();
  void Stop();
 private:
  // 处理线程
  void Process();
  bool ValidateUrl(const std::string &url);
  // 0 for consumer; 1 for producer
  bool CheckConnection(job_queue::Client *client, int type, const Options &option);
  // 构造 job queue Client
  bool BuildConnection(job_queue::Client **r_client, std::vector<job_queue::Client*> *w_clients);
  bool GetUrlStatus(const std::string &url, crawler3::url::CrawledUrlStatus *url_status);
  int32 SetPriority(double score);
  void BuildInputRequest(const crawler3::url::NewUrl &newurl,
                         double v,
                         crawler3::url::CrawledUrlStatus *url_status,
                         crawler2::crawl_queue::JobInput *input);
  bool DispatchRequest(const crawler2::crawl_queue::JobInput &input,
                       const std::vector<job_queue::Client *> &write_clients);
  inline bool is_exist_in_local_set(const std::string &url) {
    bool found = false;
    mutex_.Lock();
    auto it = urls_set_.find(url);
    if (it != urls_set_.end()) {
      found = true;
    }
    mutex_.Unlock();
    return found;
  }

  inline void add_to_local_set(const std::string &url) {
    mutex_.Lock();
    urls_set_.insert(url);
    mutex_.Unlock();
  }

  inline void check_and_clear_local_set(int32 max_set_size, int32 max_dur_in_seconds) {
    mutex_.Lock();
    int64 size = urls_set_.size();
    int64 ts = base::GetTimestamp();
    if (size > max_set_size || ts - last_clear_ts_ > max_dur_in_seconds * 1000 * 1000) {
      urls_set_.clear();
      last_clear_ts_ = ts;
    }
    mutex_.Unlock();
  }

  // 最近已经处理过的 url 集合: 避免重复处理
  std::unordered_set<std::string> urls_set_;
  // 本地缓存(url 集合) 上一次清空时间点
  int64 last_clear_ts_;

  thread::Mutex mutex_;
  redis::RedisController &redis_controler_;
  std::vector<thread::Thread *> threads_;
  const Options &in_options_;
  const std::vector<Options> &out_options_;
  web::util::UrlCategorizer url_categoryizer_;
  std::atomic<bool> stop_;
  base::PseudoRandom *pr_;
};

}  // namespace crawler
