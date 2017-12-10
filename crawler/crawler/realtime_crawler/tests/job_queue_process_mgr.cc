#include "crawler/realtime_crawler/tests/job_queue_process_mgr.h"
#include "base/thread/thread.h"
#include "base/strings/string_printf.h";

bool JobQueueProcessMgr::Start(int port, const std::string& wal) {
  CHECK(!started_);

  queue_port_ = port;
  queue_wal_ = wal;

  queue_thread_.Start(NewCallback(this, &JobQueueProcessMgr::QueueProc));
  while (!started_) {
    usleep(10000);
  }

  usleep(100000);
  return true;
}

void JobQueueProcessMgr::QueueProc() {
  // 启动 Queue
  started_ = true;
  std::string cmd
      = ::base::StringPrintf("rpc/job_queue/bin/beanstalkd -p=%d",
                             queue_port_);
  system(cmd.c_str());
}

bool JobQueueProcessMgr::Stop() {
  CHECK(started_);
  // 直接中断线程即可
}
