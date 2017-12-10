#include "crawler/realtime_crawler/css_fetch_result_handler.h"

#include <deque>

#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/thread/thread_util.h"
#include "base/common/closure.h"
#include "base/time/time.h"
#include "base/time/timestamp.h"
#include "rpc/http_counter_export/export.h"

namespace crawler2 {
namespace crawl_queue {

DEFINE_int64_counter(css_resource_handler, dropped_docs, 0, "由于队列过长丢弃的任务");

SubmitResWithCssHandler::SubmitResWithCssHandler(const Options& options)
    : options_(options)
    , stopped_(false) {
  CHECK(!options.default_queue_ip.empty());
}

SubmitResWithCssHandler::~SubmitResWithCssHandler() {
  if (!stopped_) {
    Stop();
  }
}

void SubmitResWithCssHandler::Init() {
  thread_.Start(NewCallback(this, &SubmitResWithCssHandler::SubmitProc));
}

void SubmitResWithCssHandler::Stop() {
  queue_.Close();
  thread_.Join();
  stopped_ = true;
}

void SubmitResWithCssHandler::SubmitProc() {
  thread::SetCurrentThreadName("SubmitResWithCsstHandler[SubmitProc]");
  int ret = 0;
  JobOutput* obj = NULL;
  std::string prev_ip;
  std::string prev_port;
  std::string output;
  int64 total_resource = 1;
  while ((ret = queue_.TimedTake(50, &obj)) != -1) {
    LOG_EVERY_N(INFO, 10000) << "total task send to downstream is: "
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
      COUNTERS_css_resource_handler__dropped_docs.Increase(deq.size());
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

    if (!client->PutEx(output.c_str(), output.length(),
                       obj->job().target_queue().priority(),
                       obj->job().target_queue().delay(),
                       obj->job().target_queue().ttr())) {
      LOG(ERROR) << "Failed to Put task, url: " << obj->job().detail().url();
      continue;
    }
    // 打印提交给下游 job queue 的 url, 便于追查 case
    LOG(INFO) << "added to job queue[" << ip << ":" << port << "], url: " << obj->job().detail().url();

    total_resource++;
  }
}

void SubmitResWithCssHandler::SubmitJob(const JobOutput& output) {
  JobOutput* obj = new JobOutput;
  obj->CopyFrom(output);
  queue_.Put(obj);
}

}  // namespace crawl_queue
}  // namespace crawlwer2
