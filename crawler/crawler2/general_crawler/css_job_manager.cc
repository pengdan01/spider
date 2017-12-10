#include "crawler2/general_crawler/css_job_manager.h"

#include <deque>
#include <string>

#include "base/common/logging.h"
#include "base/common/closure.h"
#include "base/thread/thread_util.h"
#include "base/encoding/line_escape.h"
#include "base/file/file_util.h"
#include "rpc/http_counter_export/export.h"
#include "web/url/url.h"

DEFINE_int64_counter(css_job_manager, job_reserved, 0, "从任务队列当中取出的任务");
DEFINE_int64_counter(css_job_manager, job_deleted, 0, "从任务队列当中删除的任务");
DEFINE_int64_counter(css_job_manager, invalid_jobs, 0, "无效的任务数量");
DEFINE_int64_counter(css_job_manager, tried_jobs, 0, "尝试抓取的任务");
DEFINE_int64_counter(css_job_manager, dropped_jobs, 0, "被丢弃的任务");
DEFINE_int64_counter(css_job_manager, connect_times, 0, "尝试连接的次数");

namespace crawler3 {

CssJobManager::CssJobManager(const Options& options, thread::BlockingQueue<JobOutput*> &job_fetched)
      : job_fetched_(job_fetched)
      , stopped_(false)
      , options_(options)
      , task_submit_(0)
      , task_done_(0)
      , task_release_(0)
      , task_invalid_(0)
      , task_delete_failed_(0)
      , task_delete_success_(0) {
}

CssJobManager::~CssJobManager() {
  if (!stopped_) {
    Stop();
  }
}

void CssJobManager::Init() {
  // 启动任务获取线程
  stopped_ = false;
  job_fetch_thread_.Start(NewCallback(this, &CssJobManager::CheckJobOutputQueue));
}

void CssJobManager::Stop() {
  job_fetch_thread_.Join();
  stopped_ = true;
}

void CssJobManager::TakeAll(CssJobManager::JobOutputQueue* jobs) {
  CHECK_NOTNULL(jobs);
  job_fetched_.TimedMultiTake(50, jobs);
}

bool CssJobManager::CheckConnection(job_queue::Client* client) {
  if (!client->IsConnected()) {
    if (!client->Connect()) {
      COUNTERS_css_job_manager__connect_times.Increase(1);
      LOG(ERROR) << "Failed to connect to queue["
                 << options_.queue_ip << ":" << options_.queue_port <<"]";
      return false;
    }
  }

  if (!client->Ping()) {
    if (!client->Reconnect()) {
      COUNTERS_css_job_manager__connect_times.Increase(1);
      LOG(ERROR) << "Failed to connection jobqueue, try again..["
                 << options_.queue_ip << ":" << options_.queue_port <<"]";
      return false;
    }
  }

  // 判断是否需要从 queue 当中获取
  if (!options_.queue_tube.empty()) {
    if (!client->Watch(options_.queue_tube)) {
      LOG(ERROR) << "Failed to call Watch(" << options_.queue_tube
                 <<") on queue[" << options_.queue_ip << ":"
                 << options_.queue_port <<"]";
      return false;
    }
  }
  return true;
}

void CssJobManager::CheckJobOutputQueue() {
  thread::SetCurrentThreadName("CssJobManager[CheckJobOutputQueue]");

  job_queue::Client client(options_.queue_ip, options_.queue_port);
  if (CheckConnection(&client)) {
    LOG(ERROR) << "Success connect to job queue[" << options_.queue_ip
              << ":" << options_.queue_port << "]";
  } else {
    LOG(ERROR) << "Failed to connect to queue[" << options_.queue_ip
              << ":" << options_.queue_port << "]";
  }

  int task_count = 0;
  while (!job_fetched_.Closed()) {
    while (true) {
      // 尝试获取任务
      job_queue::Job job;
      if (!client.ReserveWithTimeout(&job, 100)) {
        // 如果 timeout 尝试重新连接
        LOG_EVERY_N(INFO, 1000) << "Get Job timeout.";
        // 如果链接状态不可用，则直接跳出循环，等待下一轮
        LOG_IF(ERROR, !CheckConnection(&client)) << "Failed to check connection.";
        break;
      }

      COUNTERS_css_job_manager__job_reserved.Increase(1);

      if (!client.Delete(job.id)) {
        LOG(ERROR) << "Failed to delete job: " << job.id;
        continue;
      }
      COUNTERS_css_job_manager__job_deleted.Increase(1);

      // 创建任务对应数据并将任务解析处理保存于此对象当中
      task_count++;
      JobOutput* data = new JobOutput;
      if (!data->ParseFromString(job.body)
          || !data->has_res()) {
        LOG(ERROR) << "Failed to parse job: " << job.body.substr(0, 200);
        task_invalid_++;
        delete data;
        COUNTERS_css_job_manager__invalid_jobs.Increase(1);
        break;
      }

      // 打印日志, 表明该 task 已经进入抓取系统, 便于 case 追查
      LOG(INFO) << "fetched from 20071, url: " << data->job().detail().url();

      job_fetched_.Put(data);

      COUNTERS_css_job_manager__tried_jobs.Increase(1);
      LOG_EVERY_N(INFO, 100000) << "[summary] total_task fetched: " << task_count;
      continue;
    }
    // 选项当中保存的是毫秒数量， 调用 usleep 时需要乘 1000
    usleep(50 * 1000);
    LOG_EVERY_N(INFO, 1000) << "usleep for " << 50 * 1000;
  }
}

}  // namespace crawler3
