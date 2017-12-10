#include "spider/crawler/crawler.h"

#include <vector>
#include <string>

#include "base/common/sleep.h"
#include "base/common/scoped_ptr.h"
#include "base/thread/thread_util.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/file/file_util.h"
#include "web/url/url.h"
#include "spider/crawler/util.h"
#include "rpc/http_counter_export/export.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace spider{
using crawler2::crawl::CrawlRoutineStatus;
using crawler2::crawl::CrawlTask;
using crawler2::Resource;

DEFINE_int64_counter(general_crawler, task_in_queue, 0, "在候选队列当中的任务数量");
DEFINE_int64_counter(general_crawler, total_timeout_tasks, 0, "在抓取队列当中存在过长且没有得到抓取机会的任务");
DEFINE_int64_counter(general_crawler, total_task_checked, 0, "检查 task 是否符合 load control 的");
DEFINE_int64_counter(general_crawler, fetching_task, 0, "正在抓取的任务数");
DEFINE_int64_counter(general_crawler, total_task_completed, 0, "抓取完成的个数");
DEFINE_int64_counter(general_crawler, total_curl_error, 0, "出现 curl 错误的任务数");
DEFINE_int64_counter(general_crawler, total_http_2xx, 0, "返回 HTTP 2XX 的任务数");
DEFINE_int64_counter(general_crawler, total_http_4xx, 0, "返回 HTTP 4XX 的任务数");
DEFINE_int64_counter(general_crawler, total_http_5xx, 0, "返回 HTTP 5XX 的任务数");

GeneralCrawler::GeneralCrawler(
    const Options& options,
    const crawler2::crawl::ProxyManager::Options& proxy_manager_option,
    const crawler2::crawl::LoadController::Options& load_control_options,
    const crawler2::crawl::PageCrawler::Options& page_crawler_options)
  : options_(options)
  , load_controller_(load_control_options)
  , proxy_manager_(proxy_manager_option)
  , page_crawler_(page_crawler_options)
  , started_(false)
  , stopped_(false)
  , handler_(NULL) {
  // CHECK(!options.host_use_proxy_file.empty());
}

void GeneralCrawler::FetchTaskDone(crawler2::crawl::CrawlTask* task,
                                   crawler2::crawl::CrawlRoutineStatus* status,
                                   crawler2::Resource* res) {
  if (!status->success) {
    stat_.total_curl_error++;
    COUNTERS_general_crawler__total_curl_error.Increase(1);
  } else if (status->ret_code / 100 == 4) {
    stat_.total_http_4xx++;
    COUNTERS_general_crawler__total_http_4xx.Increase(1);
  } else if (status->ret_code / 100 == 5) {
    stat_.total_http_5xx++;
    COUNTERS_general_crawler__total_http_5xx.Increase(1);
  } else if (status->ret_code / 100 == 2) {
    stat_.total_http_2xx++;
    COUNTERS_general_crawler__total_http_2xx.Increase(1);
  }

  JobData* data = reinterpret_cast<JobData*>(task->extra);
  delete task;
  delete status;
  delete res;

  // 调用任务的 done: NewCallback(this, &JobManager::JobDone, job.id, data)
  if (data->done) {
    data->done->Run();
  }
}

void GeneralCrawler::Process(CrawlTask* task, CrawlRoutineStatus* status,
                              crawler2::Resource* res) {
  JobData* data = reinterpret_cast<JobData*>(task->extra);
  load_controller_.UnregisterIp(data->handle_key, ::base::GetTimestamp(),
                                status->success && status->ret_code != 403);

  stat_.task_completed++;
  COUNTERS_general_crawler__fetching_task.Decrease(1);
  COUNTERS_general_crawler__total_task_completed.Increase(1);
  Closure* done = NewCallback(this, &GeneralCrawler::FetchTaskDone,
                                 task, status, res);
  handler_->Process(task, status, res, done);
  // 放在这里减，因为后面会判断是否可以正式退出
  stat_.fetching_tasks--;

  LOG_EVERY_N(INFO, 1000) << "[summary] current fetching: "
                          << stat_.fetching_tasks
                          << " total_fetched: " << stat_.task_completed;
}

// 扫描待抓取队列, 根据压力控制决定是发起抓取, 还是回灌队列
int GeneralCrawler::CheckTasks(std::deque<crawler2::crawl::CrawlTask*>* tasks) {
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

    COUNTERS_general_crawler__total_task_checked.Increase(1);
    COUNTERS_general_crawler__task_in_queue.Decrease(1);
    stat_.task_checked++;
    stat_.task_in_queue--;
    if (ret == 0) {
      CrawlRoutineStatus* status = new CrawlRoutineStatus();
      crawler2::Resource* res = new Resource;
      Closure* done = NewCallback(this, &GeneralCrawler::Process,
                                  task, status, res);
      page_crawler_.AddTask(task, status, res, done);
      load_controller_.RegisterIp(extra->handle_key, base::GetTimestamp());
      stat_.fetching_tasks++;
      submitted_task++;

      COUNTERS_general_crawler__fetching_task.Increase(1);
    } else if (ret == -2) {
      // 如果失败次数过多，所有此  ip 上的请求
      // 将去不被丢弃
      LOG(INFO) << "Task[" << task->url << " was dropped";
      extra->status = -1;
      if (extra->done) {
        extra->done->Run();
      }
      delete task;
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
        COUNTERS_general_crawler__total_timeout_tasks.Increase(1);
        stat_.timedout_tasks++;
        extra->status = -1;
        if (extra->done) {
          // JobManager::JobDone
          extra->done->Run();
        }
        delete task;
        continue;
      } else {
        tasks->push_back(task);
        stat_.task_in_queue++;
        COUNTERS_general_crawler__task_in_queue.Increase(1);
        continue;
      }
    }
  }

  return submitted_task;
}

void GeneralCrawler::HandleJobDetailFeatures(const JobDetail &detail, CrawlTask *task) {
  int feaNum = detail.features_size(); 
  for (int i = 0; i < feaNum; ++i) {
    const FeatureItem &fea = detail.features(i); 
    const std::string &key = fea.key(); 
    const std::string &value = fea.value();
    if (key == "user_agent" && !value.empty()) {
      task->user_agent = value;
    } else if (key == "cookie" && !value.empty()) {
      task->cookie = value;
    } else if (key == "referer" && !value.empty()) {
      task->referer = value;
    }
  }
}

// 添加待抓取任务到本地队列 (deque)
void GeneralCrawler::AddTask(JobData* data) {
  const JobDetail& job_detail = data->job.detail();
  GURL gurl(job_detail.url());
  if (!gurl.is_valid() || job_detail.url().empty()) {
    LOG(ERROR) << "Invalid error...";
    data->status = -1;
    if (data->done) data->done->Run();
    return;
  }

  CrawlTask* task = new CrawlTask;
  // 默认抓取请求使用 |url|, 如果设置了 |redirect_url|, 这使用之, 但是外面显示和 压力控制
  // 均是使用 |url|
  task->url = job_detail.url();
  if (job_detail.has_redirect_url() && !job_detail.redirect_url().empty()) {
    task->url = job_detail.redirect_url();
    LOG(ERROR) << "url: " << job_detail.url() << ", real crawl url: " << job_detail.redirect_url();
  }

  task->ip = job_detail.ip();
  task->referer = job_detail.referer();
  task->resource_type = job_detail.resource_type();
  task->extra = data;

  stat_.task_in_queue++;
  COUNTERS_general_crawler__task_in_queue.Increase(1);

  // XXX(pengdan) 20140712
  // 使用代理逻辑:
  // 1. 如果单个 url 中指定使用代理;
  // 2. 如果该 url host 在 |hosts_use_proxy| 文件里面 且概率大于 |percentage_use_proxy|
  if (job_detail.has_use_proxy() && job_detail.use_proxy() == true) {
    if (job_detail.has_proxy() && !job_detail.proxy().empty()) {
      // 随机选一个代理出来
      task->proxy = SelectRandProxy(data->job);
    }
    // 如果客户端没有透传代理，这里从crawler.cfg中的代理列表里选一个
    if (task->proxy.empty()) {
      task->proxy = proxy_manager_.SelectBestProxy(::base::GetTimestamp());
    }
  } else if (hosts_use_proxy_.find(gurl.host()) != hosts_use_proxy_.end()) {
    int r = random_.GetInt(1, 100);
    if (r < options_.percentage_use_proxy) {
      task->proxy = proxy_manager_.SelectBestProxy(::base::GetTimestamp());
    }
  }
  LOG(INFO) << "url: " << job_detail.url() << ", use_proxy: " << task->proxy;

  // TODO: 尝试一下不加代理做唯一key控制的情况
  if (task->ip.empty()) {
    // data->handle_key = gurl.host() + ":" + task->proxy;
    data->handle_key = gurl.host();
  } else {
    // data->handle_key = task->ip  + ":" + task->proxy;
    data->handle_key = task->ip;
  }

  // 更加 detail 的 features 设置抓取 task 相关字段
  HandleJobDetailFeatures(job_detail, task);

  if (job_detail.has_post_body() && !job_detail.post_body().empty()) {
    task->post_body = job_detail.post_body();
    LOG(ERROR) << "url: " << job_detail.url() << ", post data: " << job_detail.post_body();
  }

  fetched_tasks_.Put(task);
}

void GeneralCrawler::CoreRoutine() {
  thread::SetCurrentThreadName("GeneralCrawler[CoreRoutine]");

  started_ = true;
  while (!fetched_tasks_.Closed()) {
    std::deque<CrawlTask*> deq;
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

void GeneralCrawler::Start(CrawlTaskHandler* handler) {
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
             << ", total size: " << lines.size();

  // 启动 page_crawler
  page_crawler_.Init();
  core_thread_.Start(NewCallback(this, &GeneralCrawler::CoreRoutine));
  while (!started_) {
    usleep(100 * 1000);
  }
}

void GeneralCrawler::Stop() {
  // 先等队列为空了，再关闭队列
  while (fetched_tasks_.Size() > 0) {
    LOG(INFO) << "wait the fetched_tasks_ queue to empty, current size is " << fetched_tasks_.Size();
    base::SleepForSeconds(1);
  }
  // 关闭队列
  fetched_tasks_.Close();
  stopped_ = true;
  LOG(INFO) << "[Stop] start to wait core_thread_ exit";
  core_thread_.Join();
  LOG(INFO) << "[Stop] core_thread_ exit successfully";

  // 还要等所有正要抓取的任务抓完
  while (!tasks_.empty()) {
    LOG(INFO) << "wait the pending crawl tasks_ queue to empty, current size is " << tasks_.size();
    // 加入剩余任务到队列当中
    CheckTasks(&tasks_);
    page_crawler_.CheckTaskStatus();
    base::SleepForSeconds(1);
  }

  // 再继续等该抓的抓完
  while (!page_crawler_.IsAllTasksCompleted()) {
    LOG(INFO) << "wait the stat_.fetching_tasks to empty, current size is " << stat_.fetching_tasks.load();
    page_crawler_.CheckTaskStatus();
    base::SleepForSeconds(1);
  }

  LOG(INFO) << "[Stop] GeneralCrawler exit successfully";
}

int GeneralCrawler::GetIdleSlotNum() const {
  return load_controller_.GetIdelSlots();
}
int GeneralCrawler::GetQueueIdelNum() const {
  return options_.max_queue_length - stat_.task_in_queue;
}
}  // namespace spider 
