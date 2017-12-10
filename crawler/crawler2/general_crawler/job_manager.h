#pragma once

#include <cstdatomic>
#include <string>
#include <deque>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "rpc/job_queue/job_queue.h"

#include "crawler2/general_crawler/crawler.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace crawler3 {
using crawler2::crawl_queue::JobDetail;

class GeneralCrawler;
class JobDetail;

class JobManager {
 public:
  struct Options {
    // job queue 相关属性
    std::string queue_ip;
    int queue_port;
    std::string queue_tube;
    // 最多尝试次数, 如果超过，job 将被丢弃
    int max_retry_times;
    int reserve_job_timeout;
    int holdon_when_jobfull;  // 单位毫秒
    Options()
        : max_retry_times(3)
        , reserve_job_timeout(600)
        , holdon_when_jobfull(100) {
    }
  };

  explicit JobManager(const Options& options);
  ~JobManager();

  void Init();
  void Stop();

  typedef std::deque<GeneralCrawler::JobData*> JobQueue;

  // 尝试从队列当中取出所有任务
  void TakeAll(JobQueue* jobs);
  void UpdateFetchNum(int num) { num_to_fetch_ = num; }
 private:
  bool CheckConnection(job_queue::Client* client);
  void PutJobProc();
  void CheckJobQueue();
  void UpdateStatusProc();

  void JobDone(int64 jobid, GeneralCrawler::JobData* job);

  // 从 job queue 中获取任务
  thread::Thread job_fetch_thread_;
  // 从需要重试的任务(如压力控制而没有被调度抓取) 回灌到 job queue
  thread::Thread job_put_thread_;
  // 存放需要回灌任务的 blocking queue
  thread::BlockingQueue<GeneralCrawler::JobData*> jobs_to_put_;
  // 存放从 job queue 中获取到的任务的 blocking queue
  thread::BlockingQueue<GeneralCrawler::JobData*> job_fetched_;

  std::atomic<bool> stopped_;
  Options options_;
  int task_submit_;
  int num_to_fetch_;
 public:
  std::atomic<int64> task_done_;
  std::atomic<int64> task_release_;
  std::atomic<int64> task_invalid_;
  std::atomic<int64> task_delete_failed_;
  std::atomic<int64> task_delete_success_;
};
}  // namespace crawler3
