#include "crawler/realtime_crawler/fetch_result_handler.h"

#include <deque>

#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/thread/thread_util.h"
#include "base/time/time.h"
#include "base/time/timestamp.h"
#include "rpc/http_counter_export/export.h"

namespace crawler2 {
namespace crawl_queue {

DEFINE_int64_counter(resource_handler, dropped_docs, 0, "由于队列过长丢弃的任务");

SubmitCrawlResultHandler::SubmitCrawlResultHandler(const Options& options)
    : status_saver_(options.status_prefix, options.saver_timespan)
    , options_(options)
    , stopped_(false) {
  CHECK(!options.default_queue_ip.empty());
}

SubmitCrawlResultHandler::~SubmitCrawlResultHandler() {
  if (!stopped_) {
    Stop();
  }
}

void SubmitCrawlResultHandler::Init() {
  thread_.Start(NewCallback(this, &SubmitCrawlResultHandler::SubmitProc));
}

void SubmitCrawlResultHandler::Stop() {
  queue_.Close();
  thread_.Join();
  stopped_ = true;
}

void SubmitCrawlResultHandler::Process(crawl::CrawlTask* task,
                                      crawl::CrawlRoutineStatus* status,
                                      Resource* res, Closure* done) {
  ScopedClosure auto_done(done);
  RealtimeCrawler::JobData* extra = (RealtimeCrawler::JobData*)task->extra;
  CHECK_NOTNULL(extra);

  // 保存状态
  JobOutput output;
  output.mutable_job()->CopyFrom(extra->job);
  output.set_completed_timestamp(::base::GetTimestamp());
  if (status->success) {
    output.mutable_res()->CopyFrom(*res);
  } else {
    output.set_detail(status->error_desc);
  }

  std::string formatted_time;
  ::base::Time t = ::base::Time::FromDoubleT(
      output.job().submitted_timestamp() / 1000.0 / 1000.0);
  if (output.job().submitted_timestamp() == 0
      || !t.ToStringInFormat("%Y%m%d%H%M%S", &formatted_time)) {
    formatted_time = "no time info";
  }

  std::string str =  ::base::StringPrintf(
      "%s\t%s\t%d\t%li\t%s\t%s\t%s\t[%s]\t%li\t%s",
      task->url.c_str(), (status->success ? "SUCCESS" : "FAILED"),
      status->ret_code, ::base::GetTimestamp(),
      status->error_desc.c_str(), (task->proxy.empty() ? "D" : "P"),
      task->proxy.c_str(), extra->handle_key.c_str(),
      extra->jobid, formatted_time.c_str());
  status_saver_.AppendLine(str, ::base::GetTimestamp());

  // TODO(pengdan): 只提交抓取成功(http response 200) 且有 res 的 job
  // 交任务
  if (status->success == true && status->ret_code == 200) {
    SubmitJob(output);
  } else {
    // XXX(pengdan): extra 这个内存已经释放(抓取内部通过 done 回调)
    LOG(INFO) << "url: " << task->url.c_str() << ", ret code: " << status->ret_code;
  }
}

void SubmitCrawlResultHandler::SubmitProc() {
  thread::SetCurrentThreadName("SubmitCrawlResultHandler[SubmitProc]");
  int ret = 0;
  JobOutput* obj = NULL;
  std::string prev_ip;
  std::string prev_port;
  std::string output;
  int64 total_resource = 1;
  int64 put_time_consume = 0;
  while ((ret = queue_.TimedTake(50, &obj)) != -1) {
    LOG_EVERY_N(INFO, 10000) << " total task send to downstream is: "
                             << total_resource << " current data in queue: "
                             << queue_.Size();
    // 如果超时
    if (ret == 0) {
      continue;
    }

    int queue_size = queue_.Size();
    if (queue_size > 100000) {
      std::deque<JobOutput*> deq;
      queue_.TryMultiTake(&deq);
      COUNTERS_resource_handler__dropped_docs.Increase(deq.size());
      while (!deq.empty()) {
        LOG(INFO) << "droped url: " << deq.front()->job().detail().url();
        delete deq.front();
        deq.pop_front();
      }
      LOG(INFO) << "Too many items in queue[" << queue_size << "], drop them directly";
    }

    scoped_ptr<JobOutput> auto_clear(obj);
    output.clear();
    std::string ip = options_.default_queue_ip;
    std::string tube = options_.default_queue_tube;
    int32 port = options_.default_queue_port;
    if (obj->job().has_target_queue()) {
      ip = obj->job().target_queue().ip();
      port = obj->job().target_queue().port();
      if (obj->job().target_queue().has_tube_name()) {
        tube = obj->job().target_queue().tube_name();
      }
    }

    if (!obj->SerializeToString(&output)) {
      LOG(ERROR) << "Failed to serialize: " << obj->DebugString() << ", url is: "
                 << obj->job().detail().url();
      continue;
    }

    job_queue::Client* client = NULL;
    std::string key = ::base::StringPrintf("%s:%d:%s", ip.c_str(), port,
                                           tube.c_str());
    auto iter = clients_.find(key);
    if (iter == clients_.end()) {
      client = new job_queue::Client(ip, port);
      clients_.insert(std::make_pair(key, client));
      if (!client->Connect()) {
        LOG(ERROR) << "Failed to connection job queue:[" << key << "]" << ", ignored url: "
                   << obj->job().detail().url();
        continue;
      }
      LOG(INFO) << "Connect to Queue[" << key << "]";
    } else {
      client = iter->second;
      // 检查链接是否正常
      if (!client->Ping()) {
        if (!client->Connect()) {
        LOG(ERROR) << "Failed to reconnect job queue: [" << key << "]" << ", ignored url: "
                   << obj->job().detail().url();
        continue;
        }
      }
    }

    if (!tube.empty()) {
      if (!client->Use(tube)) {
        LOG(ERROR) << "Failed to call Use(" << tube << ") on queue["
                   << key << "], ignored url: " << obj->job().detail().url();
        sleep(1);
        continue;
      }
    }

    int64 begin_put = ::base::GetTimestamp();
    if (!client->PutEx(output.c_str(), output.length(),
                       obj->job().target_queue().priority(),
                       obj->job().target_queue().delay(),
                       obj->job().target_queue().ttr())) {
      LOG(ERROR) << "Failed to Put task, url: " << obj->job().detail().url();
      continue;
    }
    // 打印提交给下游 job queue 的 url, 便于追查 case
    LOG(INFO) << "added to job queue[" << ip << ":" << port << "], url: " << obj->job().detail().url();

    put_time_consume += ::base::GetTimestamp() - begin_put;
    LOG_EVERY_N(INFO, 10000) << "Average Time Consume of put: "
                             << (float)put_time_consume / (float)total_resource;
    total_resource++;
  }
}

void SubmitCrawlResultHandler::SubmitJob(const JobOutput& output) {
  JobOutput* obj = new JobOutput;
  obj->CopyFrom(output);
  queue_.Put(obj);
}

}  // namespace crawl_queue
}  // namespace crawlwer2
