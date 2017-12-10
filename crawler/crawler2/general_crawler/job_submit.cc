#include "crawler2/general_crawler/job_submit.h"

#include "base/common/logging.h"
#include "base/common/closure.h"
#include "base/common/scoped_ptr.h"
#include "base/time/timestamp.h"
#include "rpc/stumy/rpc_controller.h"

namespace crawler3 {

DEFINE_int32(job_default_priority, 5, "");

bool CrawlJobSubmitter::Init() {
  if (!job_fetcher_.Connect()) {
    LOG(ERROR) << "Failed to connect to jobqueue[" << option_.queue_ip << ":" << option_.queue_port << "]";
    return false;
  }
  thread.Start(NewCallback(this, &CrawlJobSubmitter::SubmitProc));
  while (!started_) {
    usleep(10);
  }
  return true;
}

bool CrawlJobSubmitter::ConnectDetact() {
  if (!job_fetcher_.Ping() && !job_fetcher_.Reconnect()) {
    usleep(300 * 1000);
    return false;
  }

  if (!option_.queue_tube.empty()) {
    if (!job_fetcher_.Use(option_.queue_tube)) {
      LOG(ERROR) << "Failed to call Use(" << option_.queue_tube <<")";
      return false;
    }
  }

  return true;
}

void CrawlJobSubmitter::SubmitProc() {
  started_ = true;
  JobInput* item = NULL;
  ConnectDetact();
  while ((item = jobs_.Take()) != NULL) {
    scoped_ptr<JobInput> auto_item(item);

    // 尝试抓取所有，并发送
    std::string output;
    if (!item->SerializeToString(&output)) {
      LOG(ERROR) << "Failed to serialize to string.";
      continue;
    }

    // TODO(pengdan):
    int priority = FLAGS_job_default_priority;
    if (item->has_target_queue() && item->target_queue().has_priority()) {
      priority = item->target_queue().priority();
    }
    if (0 >= job_fetcher_.PutEx(output.c_str(), output.length(), priority, 0, 10)) {
      if (!ConnectDetact()) {
        sleep(1);
      }
    }
    continue;
  }
}

void CrawlJobSubmitter::Stop() {
  jobs_.Close();
  thread.Join();
}

bool CrawlJobSubmitter::AddJob(const JobInput& item) {
  JobInput *new_item = new JobInput;
  new_item->CopyFrom(item);
  jobs_.Put(new_item);
  return true;
}

bool CrawlJobSubmitter::AddJob(const JobDetail& detail) {
  JobInput *new_item = new JobInput;
  new_item->mutable_detail()->CopyFrom(detail);
  new_item->set_submitted_timestamp(base::GetTimestamp());
  jobs_.Put(new_item);
  return true;
}
}  // namespace crawler3

