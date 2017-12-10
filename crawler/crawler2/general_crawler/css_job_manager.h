#pragma once

#include <cstdatomic>
#include <string>
#include <deque>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "rpc/job_queue/job_queue.h"

using crawler2::crawl_queue::JobInput;
using crawler2::crawl_queue::JobOutput;

namespace crawler3 {

class CssJobManager {
 public:
  struct Options {
    // job queue 相关属性
    std::string queue_ip;
    int queue_port;
    std::string queue_tube;
    Options() {}
  };

  explicit CssJobManager(const Options& options, thread::BlockingQueue<JobOutput*> &job_fetched);
  ~CssJobManager();

  void Init();
  void Stop();

  typedef std::deque<JobOutput*> JobOutputQueue;

  // 尝试从队列当中取出所有任务
  void TakeAll(JobOutputQueue* jobs);
 private:
  bool CheckConnection(job_queue::Client* client);
  void PutJobProc();
  void CheckJobOutputQueue();
  void UpdateStatusProc();

  // 从 job queue 中获取任务
  ::thread::Thread job_fetch_thread_;
  // 存放从 job queue 中获取到的任务的 blocking queue
  // queue 中的内存, 需要使用者进行释放, 否则: 内存泄漏
  ::thread::BlockingQueue<JobOutput*> &job_fetched_;

  std::atomic<bool> stopped_;
  Options options_;
  int task_submit_;
 public:
  std::atomic<int64> task_done_;
  std::atomic<int64> task_release_;
  std::atomic<int64> task_invalid_;
  std::atomic<int64> task_delete_failed_;
  std::atomic<int64> task_delete_success_;
};
}  // namespace crawler3
