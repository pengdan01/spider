#pragma once

#include <list>
#include <set>
#include <deque>
#include <string>

#include "base/thread/blocking_queue.h"
#include "base/thread/thread.h"
#include "base/random/pseudo_random.h"
#include "rpc/stumy/stumy_server.h"
#include "rpc/stumy/rpc_channel.h"
#include "serving/serving_proto/serving.pb.h"
#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/load_controller.h"
#include "crawler/crawl/proxy_manager.h"
#include "crawler/crawl/crawl_task.h"
#include "crawler/proto2/resource.pb.h"

#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace crawler3 {
using crawler2::crawl_queue::JobInput;
using crawler2::crawl_queue::JobOutput;
using crawler2::crawl_queue::JobDetail;

class CrawlTaskHandler {
 public:
  virtual ~CrawlTaskHandler() {}
  virtual void Process(crawler2::crawl::CrawlTask* task,
                       crawler2::crawl::CrawlRoutineStatus* status, crawler2::Resource* res,
                       Closure* done) = 0;
};

struct CrawlerStat {
  std::atomic<int> fetching_tasks;      // 当前正在抓取的任务
  std::atomic<int64> task_in_queue;     // 在队列
  std::atomic<int64> timedout_tasks;
  std::atomic<int64> task_checked;
  std::atomic<int64> task_completed;    // 抓取完成的任务
  std::atomic<int64> total_curl_error;  // 由于 curl 错误导致的失败
  std::atomic<int64> total_http_2xx;    //
  std::atomic<int64> total_http_4xx;    //
  std::atomic<int64> total_http_5xx;    //

  CrawlerStat()
      : fetching_tasks(0)
      , task_in_queue(0)
      , task_checked(0)
      , task_completed(0)
      , total_curl_error(0)
      , total_http_2xx(0)
      , total_http_4xx(0)
      , total_http_5xx(0) {
  }
};

class GeneralCrawler {
 public:
  // proxy
  struct ServerInfo {
    std::string ip;
    int port;
  };

  struct Options {
    // 队列当中最多包含的数量
    int64 max_queue_length;
    // 任务呆在队列当中的最长时间, 单位: 毫秒
    int64 max_duration_inqueue;
    int percentage_use_proxy;   // 99 代表 99%
    std::string host_use_proxy_file;
    Options()
        : max_queue_length(102400)
        , max_duration_inqueue(10 * 60 * 1000)
        , percentage_use_proxy(50) {
    }
  };

  GeneralCrawler(const Options& options,
                  const crawler2::crawl::ProxyManager::Options& proxy_manager_option,
                  const crawler2::crawl::LoadController::Options& load_control_options,
                  const crawler2::crawl::PageCrawler::Options& page_cralwer_options);

  struct JobData {
    int64 jobid;
    JobInput job;
    int status;  // 0 表示成功， 其他表示失败
    std::string desc;
    Closure* done;
    int64 timestamp;  // 加入任务队列的时间
    std::string handle_key;  // 用于区别使用那个字段来分配 handle, 可以使用 ip, url 等
    JobData() : status(0), done(NULL) {
    }

    ~JobData() {
    }
  };

  // 增加一个任务
  void AddTask(JobData* job);
  void Start(CrawlTaskHandler* handler);
  void Loop();
  void Stop();

  int GetIdleSlotNum() const;
  int GetQueueIdelNum() const;
 private:
  // 更新数据
  struct TaskStatus {
    crawler2::Resource* res;
    crawler2::crawl::CrawlTask* task;
    crawler2::crawl::CrawlRoutineStatus* crawl_status;
    int64 timestamp;  // 插入队列的时间
    TaskStatus() {
      crawl_status = NULL;
      res = NULL;
      task = NULL;
    }
    ~TaskStatus() {
      CHECK_NOTNULL(res);
      CHECK_NOTNULL(crawl_status);
      CHECK_NOTNULL(task);
      delete crawl_status;
      delete task;
      delete res;
    }
  };

  int CheckTasks(std::deque<crawler2::crawl::CrawlTask*>* tasks);
  void Process(crawler2::crawl::CrawlTask* task,
               crawler2::crawl::CrawlRoutineStatus* status, crawler2::Resource* res);

  // core routine
  void CoreRoutine();
  void StatusProc();
  void SubmitProc();
  void FetchTaskDone(crawler2::crawl::CrawlTask* task,
                     crawler2::crawl::CrawlRoutineStatus* status,
                     crawler2::Resource* res);

  Options options_;
  std::set<std::string> sites_use_proxy_;
  // 变量名字取得不太友好, 这个是待抓取的任务, 而不是已经抓取
  thread::BlockingQueue<crawler2::crawl::CrawlTask*> fetched_tasks_;
  std::deque<crawler2::crawl::CrawlTask*> tasks_;
  thread::Thread core_thread_;

  crawler2::crawl::LoadController load_controller_;
  crawler2::crawl::ProxyManager proxy_manager_;
  crawler2::crawl::PageCrawler page_crawler_;

  // host 存在于列表当中的 使用代理抓取
  std::set<std::string> hosts_use_proxy_;

  // 在任务队列当中的任务
  std::atomic<bool> started_;
  std::atomic<bool> stopped_;
  CrawlTaskHandler* handler_;

  base::PseudoRandom random_;
 public:
  //=====================================
  // 统计数据
  CrawlerStat stat_;
};
}  // namespace crawler3
