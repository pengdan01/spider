#pragma once


#include <cstdatomic>

#include <string>

#include "rpc/job_queue/job_queue.h"
#include "base/thread/thread.h"


// 此类负责管理 Jobqueue 的启动进程，简化 UT 对于 JobQueue
// 的相关工作
class JobQueueProcessMgr {
 public:
  JobQueueProcessMgr()
      : started_(false) {
  }

  bool Start(int port, const std::string& wal);
  bool Stop();
 private:
  void QueueProc();
  std::atomic<bool> started_;
  std::string queue_wal_;
  int queue_port_;
};


