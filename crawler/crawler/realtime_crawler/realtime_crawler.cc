#include "crawler/realtime_crawler/realtime_crawler.h"

#include <vector>
#include <string>

#include "base/common/scoped_ptr.h"
#include "base/thread/thread_util.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/file/file_util.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler/realtime_crawler/extra.h"
#include "rpc/http_counter_export/export.h"

#include "web/url/url.h"

namespace crawler2 {
namespace crawl_queue {
using ::crawler2::crawl::CrawlRoutineStatus;
using ::crawler2::crawl::CrawlTask;

DEFINE_int64_counter(realtime_crawler, task_in_queue, 0, "在候选队列当中的任务数量");
DEFINE_int64_counter(realtime_crawler, timedout_tasks, 0, "在抓取队列当中存在过长且没有得到抓取机会的任务");
DEFINE_int64_counter(realtime_crawler, task_checked, 0, "检查 task 是否符合 load control 的");
DEFINE_int64_counter(realtime_crawler, fetching_task, 0, "正在抓取的任务数");
DEFINE_int64_counter(realtime_crawler, task_completed, 0, "抓取完成的个数");
DEFINE_int64_counter(realtime_crawler, curl_error, 0, "出现 curl 错误的任务数");
DEFINE_int64_counter(realtime_crawler, http_2xx, 0, "返回 HTTP 2XX 的任务数");
DEFINE_int64_counter(realtime_crawler, http_4xx, 0, "返回 HTTP 4XX 的任务数");
DEFINE_int64_counter(realtime_crawler, http_5xx, 0, "返回 HTTP 5XX 的任务数");

DEFINE_int32(all_by_proxy, 0, "0: 按照配置使用代理 ， 1: 全部使用代理");

RealtimeCrawler::RealtimeCrawler(
    const Options& options,
    const crawl::ProxyManager::Options& proxy_manager_option,
    const crawl::LoadController::Options& load_control_options,
    const crawl::PageCrawler::Options& page_crawler_options)
  : options_(options)
  , load_controller_(load_control_options)
  , proxy_manager_(proxy_manager_option)
  , page_crawler_(page_crawler_options)
  , started_(false)
  , stopped_(false)
  , handler_(NULL) {
  // CHECK(!options.host_use_proxy_file.empty());
}

void RealtimeCrawler::FetchTaskDone(::crawler2::crawl::CrawlTask* task,
                     ::crawler2::crawl::CrawlRoutineStatus* status,
                                    Resource* res) {
  if (!status->success) {
    stat_.total_curl_error++;
    COUNTERS_realtime_crawler__curl_error.Increase(1);
  } else if (status->ret_code / 100 == 4) {
    stat_.total_http_4xx++;
    COUNTERS_realtime_crawler__http_4xx.Increase(1);
  } else if (status->ret_code / 100 == 5) {
    stat_.total_http_5xx++;
    COUNTERS_realtime_crawler__http_5xx.Increase(1);
  } else if (status->ret_code / 100 == 2) {
    stat_.total_http_2xx++;
    COUNTERS_realtime_crawler__http_2xx.Increase(1);
  }

  JobData* data = reinterpret_cast<JobData*>(task->extra);
  delete task;
  delete status;
  delete res;

  // 调用任务的  done
  if (data->done) {
    data->done->Run();
  }
}

void RealtimeCrawler::Process(CrawlTask* task, CrawlRoutineStatus* status,
                              Resource* res) {
  JobData* data = reinterpret_cast<JobData*>(task->extra);
  load_controller_.UnregisterIp(data->handle_key, ::base::GetTimestamp(),
                                status->success && status->ret_code != 403);
  stat_.fetching_tasks--;
  // XXX(pengdan): 应该是 完成任务数加一, 而不是 --
  stat_.task_completed++;
  COUNTERS_realtime_crawler__fetching_task.Decrease(1);
  COUNTERS_realtime_crawler__task_completed.Increase(1);
  Closure* done = NewCallback(this, &RealtimeCrawler::FetchTaskDone,
                                 task, status, res);
  handler_->Process(task, status, res, done);

  LOG_EVERY_N(INFO, 1000) << "[summary] current fetching: "
                          << stat_.fetching_tasks
                          << " total_fetched: " << stat_.task_completed;
}

int RealtimeCrawler::CheckTasks(std::deque<crawl::CrawlTask*>* tasks) {
  int submitted_task = 0;
  const int task_size = (int)tasks->size();
  int64 current_timestamp = ::base::GetTimestamp();
  for (int i = 0; i < task_size; ++i) {
    if (load_controller_.GetIdelSlots() <= 0) {
      return 0;
    }

    if (tasks->empty()) {
      break;
    }

    CrawlTask* task = tasks->front();
    JobData* extra = reinterpret_cast<JobData*>(task->extra);
    tasks->pop_front();
    int64 ret = load_controller_.CheckFetch(extra->handle_key,
                                            base::GetTimestamp());

    COUNTERS_realtime_crawler__task_checked.Increase(1);
    COUNTERS_realtime_crawler__task_in_queue.Decrease(1);
    stat_.task_checked++;
    stat_.task_in_queue--;
    if (ret == 0) {
      CrawlRoutineStatus* status = new CrawlRoutineStatus();
      Resource* res = new Resource;
      Closure* done = NewCallback(this, &RealtimeCrawler::Process,
                                  task, status, res);
      page_crawler_.AddTask(task, status, res, done);
      load_controller_.RegisterIp(extra->handle_key, base::GetTimestamp());
      stat_.fetching_tasks++;
      submitted_task++;

      COUNTERS_realtime_crawler__fetching_task.Increase(1);
    } else if (ret == -2) {
      // 如果失败次数过多，所有此  ip 上的请求
      // 将去不被丢弃
      LOG(INFO) << "Task[" << task->url << " was dropped";
      extra->status = -1;
      if (extra->done) {
        extra->done->Run();
      }
      continue;
    } else {
      LOG_EVERY_N(INFO, 100000)
          << "[summary] Drop back to queue because of load control";
      // 如果由于 load controller 无法进入爬去队列
      // 则从新放入到队列当中

      // 检查队列当中的任务在队列当中存在了多久，如果超过一段时间
      // 则直接丢弃
      if (current_timestamp - extra->timestamp
          > 1000 * options_.max_duration_inqueue) {
        LOG_EVERY_N(ERROR, 100000)
            << "the job has been in queue too long["
            << current_timestamp << ":" << extra->timestamp;
        COUNTERS_realtime_crawler__timedout_tasks.Increase(1);
        stat_.timedout_tasks++;
        extra->status = -1;
        if (extra->done) {
          extra->done->Run();
        }
        continue;
      } else {
        tasks->push_back(task);
        stat_.task_in_queue++;
        COUNTERS_realtime_crawler__task_in_queue.Increase(1);
      }
      continue;
    }
  }

  return submitted_task;
}

void RealtimeCrawler::AddTask(JobData* data) {
  const JobDetail& job_detail = data->job.detail();
  GURL gurl(job_detail.url());
  if (!gurl.is_valid() || job_detail.url().empty()) {
    LOG(ERROR) << "Invalid error...";
    data->status = -1;
    if (data->done) data->done->Run();
    return;
  }

  crawl::CrawlTask* task = new crawl::CrawlTask;
  task->url = job_detail.url();
  task->ip = job_detail.ip();
  task->referer = job_detail.referer();
  task->resource_type = job_detail.resource_type();
  task->extra = data;
  stat_.task_in_queue++;
  COUNTERS_realtime_crawler__task_in_queue.Increase(1);

  if (FLAGS_all_by_proxy == 1 ||  hosts_use_proxy_.find(gurl.host()) != hosts_use_proxy_.end()) {
    int r = random_.GetInt(1, 100);
    if (r < options_.percentage_use_proxy || FLAGS_all_by_proxy == 1) {
      task->proxy = proxy_manager_.SelectBestProxy(::base::GetTimestamp());
    }
  }

    // 分配 handle key
  if (task->ip.empty()) {
    std::string tld;
    std::string domain;
    std::string subdomain;
    if (!web::ParseHost(gurl.host(), &tld, &domain, &subdomain)) {
      // LOG(ERROR) << "Failed to parseHost:" << gurl.host();
      data->handle_key = gurl.host() + task->proxy;
      // LOG(ERROR) << data->job.DebugString();
    } else{
      data->handle_key = domain + ":" + task->proxy;
    }
  } else {
    data->handle_key = task->ip  + task->proxy;
  }

  fetched_tasks_.Put(task);
}

void RealtimeCrawler::CoreRoutine() {
  thread::SetCurrentThreadName("RealtimeCrawler[CoreRoutine]");

  started_ = true;
  while (!fetched_tasks_.Closed()) {
    std::deque<crawl::CrawlTask*> deq;
    fetched_tasks_.TryMultiTake(&deq);

    while (!deq.empty()) {
      tasks_.push_back(deq.front());
      deq.pop_front();
    }

    // 加入到队列当中
    CheckTasks(&tasks_);
    page_crawler_.CheckTaskStatus();
  }
}

void RealtimeCrawler::Start(CrawlTaskHandler* handler) {
  CHECK_NOTNULL(handler);
  CHECK(handler_ == NULL);
  handler_ = handler;

  LOG(INFO) << options_.percentage_use_proxy  << "% will use proxy.";
  LOG(INFO) << "MAX " << options_.max_duration_inqueue
            << " item in queue. ";

  proxy_manager_.Init();

  // 载入使用代理的列表文件
  std::vector<std::string> lines;
  CHECK(base::file_util::ReadFileToLines(
      base::FilePath(options_.host_use_proxy_file), &lines))
      << "Failed to read proxy list file: " << options_.host_use_proxy_file;
  for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
    if (iter->empty()) continue;
    hosts_use_proxy_.insert(*iter);
  }

  LOG(INFO) << "Load HostUsingProxy[" << options_.host_use_proxy_file <<"]"
             << " total size is: " << lines.size();

  // 启动 page_crawler
  page_crawler_.Init();
  core_thread_.Start(NewCallback(this, &RealtimeCrawler::CoreRoutine));
  while (!started_) {
    usleep(100 * 1000);
  }
}


void RealtimeCrawler::Loop() {
  while (!stopped_) {
    usleep(100 * 1000);
  }
}

void RealtimeCrawler::Stop() {
  // 关闭所有 blocking queue 并等待所有线程关闭
  fetched_tasks_.Close();
  stopped_ = true;
  core_thread_.Join();
}

int RealtimeCrawler::GetIdleSlotNum() const {
  return load_controller_.GetIdelSlots();
}
int RealtimeCrawler::GetQueueIdelNum() const {
  return options_.max_queue_length - stat_.task_in_queue;
}

}  // namespace crawl_queue {
}  // namespace crawler2
