#include "crawler2/general_crawler/job_manager.h"

#include "base/common/closure.h"
#include "base/thread/thread_util.h"
#include "base/encoding/line_escape.h"
#include "base/file/file_util.h"
#include "rpc/http_counter_export/export.h"
#include "web/url/url.h"

DEFINE_int64_counter(job_manager, job_reserved, 0, "从任务队列当中取出的任务");
DEFINE_int64_counter(job_manager, job_deleted, 0, "从任务队列当中删除的任务");
DEFINE_int64_counter(job_manager, invalid_jobs, 0, "无效的任务数量");
DEFINE_int64_counter(job_manager, tried_jobs, 0, "尝试抓取的任务");
DEFINE_int64_counter(job_manager, dropped_jobs, 0, "被丢弃的任务");
DEFINE_int64_counter(job_manager, job_put_back, 0, "由于某些原因重新返回的任务");
DEFINE_int64_counter(job_manager, job_put_back_fail, 0, "返回的任务失败数");
DEFINE_int64_counter(job_manager, connect_times, 0, "尝试连接的次数");

DEFINE_int32(job_retry_times, 2, "each job(urls) will be crawled at most |job_retry_times| + 1 times");

namespace crawler3 {

JobManager::JobManager(const Options& options)
    : stopped_(false)
      , options_(options)
      , task_submit_(0)
      , num_to_fetch_(0)
      , task_done_(0)
      , task_release_(0)
      , task_invalid_(0)
      , task_delete_failed_(0)
      , task_delete_success_(0) {
        CHECK_GT(options_.reserve_job_timeout, 0);
      }

JobManager::~JobManager() {
  if (!stopped_) {
    Stop();
  }
}

void JobManager::Init() {
  // 启动任务获取线程
  stopped_ = false;
  job_fetch_thread_.Start(NewCallback(this, &JobManager::CheckJobQueue));
  job_put_thread_.Start(NewCallback(this, &JobManager::PutJobProc));
  return;
}

void JobManager::Stop() {
  jobs_to_put_.Close();
  job_fetch_thread_.Join();
  job_put_thread_.Join();
  stopped_ = true;
}

void JobManager::TakeAll(JobManager::JobQueue* jobs) {
  CHECK_NOTNULL(jobs);
  job_fetched_.TimedMultiTake(50, jobs);
}

void JobManager::PutJobProc() {
  thread::SetCurrentThreadName("JobManager[PutJobProc]");

  job_queue::Client *client = NULL;
  client = new job_queue::Client(options_.queue_ip, options_.queue_port);
  if (CheckConnection(client)) {
    LOG(ERROR) << "Success connect to job queue[" << options_.queue_ip
               << ":" << options_.queue_port << "]";
  } else {
    // TODO(pengdan): 如果连不上, 应该 FATAL
    LOG(ERROR) << "Failed to connect to queue[" << options_.queue_ip
               << ":" << options_.queue_port << "]";
  }

  int64 totoal_put_back = 0;
  std::string serialized;
  GeneralCrawler::JobData* job = NULL;
  while ((job = jobs_to_put_.Take()) != NULL) {
    serialized.clear();
    scoped_ptr<GeneralCrawler::JobData> auto_job(job);
    job->job.set_tried_times(job->job.tried_times() + 1);

    LOG_EVERY_N(INFO, 1000) << "[summary] total put back: " << totoal_put_back;
    if (!job->job.SerializeToString(&serialized)) {
      LOG(ERROR) << "Failed to Serialize JobInput to string.";
      continue;
    }

    // XXX(pengdan): 需要指定优先级
    int default_priority = 5;
    if (job->job.has_target_queue() && job->job.target_queue().has_priority()) {
      default_priority = job->job.target_queue().priority();
    }
    int64 ret = client->PutEx(serialized.c_str(), serialized.length(), default_priority, 0, 60);
    if (ret <= 0) {
      LOG(ERROR) << "Failed in client.PutEx(), url: " << job->job.detail().url() << ", ret code: " << ret;
      COUNTERS_job_manager__job_put_back_fail.Increase(1);
      delete client;
      client = new job_queue::Client(options_.queue_ip, options_.queue_port);
      LOG_IF(ERROR, !CheckConnection(client)) << "connect to job queue fail";
    } else {
      totoal_put_back++;
      COUNTERS_job_manager__job_put_back.Increase(1);
      VLOG(5) << "BACK to 20080/19000, url: " << job->job.detail().url();
    }
  }
}

bool JobManager::CheckConnection(job_queue::Client* client) {
  if (!client->IsConnected()) {
    if (!client->Connect()) {
      COUNTERS_job_manager__connect_times.Increase(1);
      LOG(ERROR) << "Failed to connect to queue["
                 << options_.queue_ip << ":" << options_.queue_port <<"]";
      return false;
    }
  }

  if (!client->Ping()) {
    if (!client->Reconnect()) {
      COUNTERS_job_manager__connect_times.Increase(1);
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

void JobManager::CheckJobQueue() {
  thread::SetCurrentThreadName("JobManager[CheckJobQueue]");

  job_queue::Client *client = new job_queue::Client(options_.queue_ip, options_.queue_port);
  if (CheckConnection(client)) {
    LOG(ERROR) << "Success connect to job queue[" << options_.queue_ip
               << ":" << options_.queue_port << "]";
  } else {
    LOG(FATAL) << "Failed to connect to queue[" << options_.queue_ip
               << ":" << options_.queue_port << "]";
  }

  int task_count = 0;
  while (!jobs_to_put_.Closed()) {
    std::string str;
    // 只有当 当前任务队列小于需要抓取的最大数量 才进行抓取
    while (job_fetched_.Size() < num_to_fetch_) {
      // 尝试获取任务
      job_queue::Job job;
      if (!client->ReserveWithTimeout(&job, options_.reserve_job_timeout)) {
        // 如果 timeout 尝试重新连接
        LOG_EVERY_N(INFO, 1000) << "Get Job timeout.";

        if (!CheckConnection(client)) {
          // 如果链接状态不可用，则直接跳出循环，等待下一轮
          LOG(ERROR) << "Failed to check connection, re";
          delete client;
          client = new job_queue::Client(options_.queue_ip, options_.queue_port);
          break;
        }
        break;
      }

      COUNTERS_job_manager__job_reserved.Increase(1);

      if (!client->Delete(job.id)) {
        LOG(ERROR) << "Failed to delete job: " << job.id;
        continue;
      }
      COUNTERS_job_manager__job_deleted.Increase(1);

      // 创建任务对应数据并将任务解析处理保存于此对象当中
      task_count++;
      GeneralCrawler::JobData* data = new GeneralCrawler::JobData;
      if (!data->job.ParseFromString(job.body)
          || data->job.detail().url().empty()) {
        LOG(ERROR) << "Failed to parse job: " << job.body.substr(0, 200);
        task_invalid_++;
        delete data;
        COUNTERS_job_manager__invalid_jobs.Increase(1);
        break;
      }

      // 判断任务是否合法如果不合法，删除任务并限制一段任务文本
      GURL gurl(data->job.detail().url());
      if (!gurl.is_valid()) {
        LOG(ERROR) << "invalid url: "
                   << data->job.detail().url().substr(0, 200);
        task_invalid_++;
        delete data;
        COUNTERS_job_manager__invalid_jobs.Increase(1);
        break;
      }

      // 超过最大尝试次数, 直接丢弃
      // 修改重试次数为 1, 即: 抓取一次失败后, 就扔掉. 因为重试次数过多, 会导致队列堆积,影响其他 url
      // 的收录时效性
      // 0903: 不重试时, 抓取成功率降低 7% 左右, 所以跳回 2
      if (data->job.tried_times() == FLAGS_job_retry_times) {
        COUNTERS_job_manager__dropped_jobs.Increase(1);
        LOG(ERROR) << "drop url:\t" <<  data->job.detail().url()
                   << "\tretry_times:" << data->job.tried_times();
        delete data;
        continue;
      }

      // 将任务放置到抓取队列当中
      data->jobid = job.id;
      data->done = ::NewCallback(this, &JobManager::JobDone, job.id, data);
      data->timestamp = ::base::GetTimestamp();
      job_fetched_.Put(data);

      // 打印日志, 表明该 task 已经进入抓取系统, 便于 case 追查
      LOG(INFO) << "fetched from 20080/19000, url: " << data->job.detail().url();

      COUNTERS_job_manager__tried_jobs.Increase(1);
      LOG_EVERY_N(INFO, 100000) << "[summary] total_task fetched: " << task_count;
      continue;
    }

    // 如果队列已经满了， 需要休息一段时间
    // 选项当中保存的是毫秒数量， 调用 usleep 时需要乘 1000
    usleep(options_.holdon_when_jobfull * 1000);
    LOG_EVERY_N(INFO, 1000) << "when queue is full, usleep for "
                            << options_.holdon_when_jobfull * 1000;
  }
}

void JobManager::JobDone(int64 jobid, GeneralCrawler::JobData* job) {
  if (job->status == 0) {
    task_done_++;
    delete job;
  } else {
    task_release_++;
    jobs_to_put_.Put(job);
  }

  LOG_EVERY_N(INFO, 10000) << "[summary] job completed: " << task_done_
                           << ", job release: " << task_release_
                           << ", job invalid: " << task_invalid_
                           << ", job deleted: " << task_delete_success_
                           << ", job failed to delete: " << task_delete_failed_;
}
}  // namespace crawler3
