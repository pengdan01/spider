#include "crawler2/general_crawler/fetch_result_handler.h"

#include <deque>

#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/thread/thread_util.h"
#include "base/hash_function/url.h"
#include "base/time/time.h"
#include "base/time/timestamp.h"
#include "rpc/http_counter_export/export.h"

#include "crawler2/general_crawler/redis_const.h"

namespace crawler3 {

DEFINE_bool(report_crawl_status_to_redis, true, "将抓取状态信息报告给 redis 集群, 调度模块会用到这些信息");

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
  if (FLAGS_report_crawl_status_to_redis) {
    redis_util::LoadRedisServerList(&server_ip_);
    CHECK_GT(server_ip_.size(), 0u);
    for (int i = 0; i < (int)server_ip_.size(); ++i) {
      redis_clients_[i] = new redis::Client(server_ip_[i].first, server_ip_[i].second);
      CHECK(redis_clients_[i]->Connect()) << "Failed to connect redis[" << server_ip_[i].first<< ":"
                                          << server_ip_[i].second << "]";
      LOG(INFO) << "connect redis[" << server_ip_[i].first << ":" << server_ip_[i].second << "] ok.";
    }

    report_status_thread_.Start(NewCallback(this, &SubmitCrawlResultHandler::ReportStatusProc));
  }
  thread_.Start(NewCallback(this, &SubmitCrawlResultHandler::SubmitProc));
}

void SubmitCrawlResultHandler::Stop() {
  queue_.Close();
  thread_.Join();
  if (FLAGS_report_crawl_status_to_redis) {
    status_queue_.Close();
    report_status_thread_.Join();
  }
  stopped_ = true;
}

void SubmitCrawlResultHandler::Process(crawler2::crawl::CrawlTask* task,
                                      crawler2::crawl::CrawlRoutineStatus* status,
                                      crawler2::Resource* res, Closure* done) {
  ScopedClosure auto_done(done);
  GeneralCrawler::JobData* extra = (GeneralCrawler::JobData*)task->extra;
  CHECK_NOTNULL(extra);

  // 保存状态
  JobOutput output;
  output.mutable_job()->CopyFrom(extra->job);
  output.set_completed_timestamp(base::GetTimestamp());
  if (status->success) {
    output.mutable_res()->CopyFrom(*res);
  } else {
    output.set_detail(status->error_desc);
  }

  std::string formatted_time;
  base::Time t = ::base::Time::FromDoubleT(
      output.job().submitted_timestamp() / 1000.0 / 1000.0);
  if (output.job().submitted_timestamp() == 0
      || !t.ToStringInFormat("%Y%m%d%H%M%S", &formatted_time)) {
    formatted_time = "no time info";
  }

  int64 ts = base::GetTimestamp();
  std::string str = base::StringPrintf(
      "%s\t%s\t%d\t%li\t%s\t%s\t%s\t[%s]\t%li\t%s",
      task->url.c_str(), (status->success ? "SUCCESS" : "FAILED"),
      status->ret_code, ts,
      status->error_desc.c_str(), (task->proxy.empty() ? "D" : "P"),
      task->proxy.c_str(), extra->handle_key.c_str(),
      extra->jobid, formatted_time.c_str());
  status_saver_.AppendLine(str, base::GetTimestamp());

  // 只提交抓取成功(http response 200) 且有 res 的 job
  if (status->success == true && status->ret_code == 200) {
    SubmitJob(output);
  } else {
    // XXX(pengdan): extra 这个内存已经释放(抓取内部通过 done 回调)
    LOG(INFO) << "url: " << task->url.c_str() << ", ret code: " << status->ret_code;
  }

  // report status to redis
  if (FLAGS_report_crawl_status_to_redis) {
    url::CrawledUrlStatus report_status;
    report_status.set_sign(base::CalcUrlSign(task->url.c_str(), task->url.size()));
    report_status.set_clicked_url(task->url);
    report_status.set_latest_crawl_timestamp(ts);
    report_status.set_is_crawled(status->success);
    report_status.set_depth(output.job().detail().depth());
    report_status.set_priority(output.job().detail().priority());

    SubmitStatus(report_status);
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
    LOG_EVERY_N(INFO, 500) << " total task send to downstream is: "
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
    if (obj->job().has_target_queue() && obj->job().target_queue().has_ip() &&
        !obj->job().target_queue().ip().empty()) {
      ip = obj->job().target_queue().ip();
    }
    if (obj->job().has_target_queue() && obj->job().target_queue().has_port() &&
        obj->job().target_queue().port() > 0) {
      port = obj->job().target_queue().port();
    }
    if (obj->job().has_target_queue() && obj->job().target_queue().has_tube_name() &&
        !obj->job().target_queue().tube_name().empty()) {
      tube = obj->job().target_queue().tube_name();
    }

    if (!obj->SerializeToString(&output)) {
      LOG(ERROR) << "Failed to serialize: " << obj->DebugString() << ", url is: "
                 << obj->job().detail().url();
      continue;
    }

    job_queue::Client* client = NULL;
    std::string key = base::StringPrintf("%s:%d:%s", ip.c_str(), port,
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

    int64 begin_put = base::GetTimestamp();
    if (0 >= client->PutEx(output.c_str(), output.length(),
                           obj->job().target_queue().priority(),
                           obj->job().target_queue().delay(),
                           obj->job().target_queue().ttr())) {
      LOG(ERROR) << "Failed to Put task, url: " << obj->job().detail().url();
      continue;
    }
    // 打印提交给下游 job queue 的 url, 便于追查 case
    LOG(INFO) << "added to job queue[" << ip << ":" << port << "], url: " << obj->job().detail().url();

    put_time_consume += base::GetTimestamp() - begin_put;
    LOG_EVERY_N(INFO, 10000) << "Average Time Consume of put: "
                             << (float)put_time_consume / (float)total_resource;
    total_resource++;
  }
}

void SubmitCrawlResultHandler::ReportStatusProc() {
  thread::SetCurrentThreadName("SubmitCrawlResultHandler[ReportStatusProc]");

  int ret = 0;
  int64 total_report = 0;
  int64 total_failed = 0;
  int64 put_time_consume = 0;

  std::vector<std::string> fields;
  fields.push_back(kCrawlUrlStatusField);

  std::deque<url::CrawledUrlStatus*> deq;
  while ((ret = status_queue_.MultiTake(&deq)) != -1) {
    LOG_EVERY_N(INFO, 10000) << " total status report to redis: " << total_report;
    // report to redis
    // XXX(pengdan): 原本打算是: 对于抓取成功的 url, 直接写入 redis; 对于抓取失败的 url, 先查询
    // redis 得到 上一次抓取时 连续失败次数, 然后将其值加一, 在写入 redis. 但是 这样相当于要两次
    // 访问 redis, 而且对 连续失败次数 这个数据页没有明确使用策略, 先暂且放一放
    // 当前策略: 不过抓取成功还是失败, 直接将最新的抓取状态写入 redis
    int64 begin_put = base::GetTimestamp();
    for (auto it = deq.begin(); it != deq.end(); ++it) {
      std::string output;
      if (!(*it)->SerializeToString(&output)) {
        LOG(ERROR) << "(*it)->SerializeToString() fail";
        continue;
      }
      int32 id = GetRedisServerId((*it)->sign());
      redis::Client* client = redis_clients_[id];
      CHECK_NOTNULL(client);

      std::vector<std::string> keys;
      keys.push_back(base::StringPrintf("%lu", (*it)->sign()));
      std::vector<std::string> values;
      values.push_back(output);

      if (!redis_util::TryRedisPushEx(client, keys, fields, values, 3600*24*90)) {
        ++total_failed;
        LOG(ERROR) << "Failed in TryRedisPush(), redis server[" << server_ip_[id].first
                   << ":" << server_ip_[id].second << "]" << ", failed count: " << total_failed;
        continue;
      } else {
        // 打印提交给 redis 的 url, 便于追查 case
        ++total_report;
        LOG(INFO) << "reported to redis[" << server_ip_[id].first<< ":" << server_ip_[id].second
                  << "], url: " << (*it)->clicked_url();
      }
    }
    put_time_consume += base::GetTimestamp() - begin_put;
    LOG_EVERY_N(INFO, 10000) << "Average Time Consume of put: "
                             << (double)put_time_consume / (double)(1+total_report);
    // release memory
    for (auto it = deq.begin(); it != deq.end(); ++it) {
      delete *it;
    }
    deq.clear();
  }
}

void SubmitCrawlResultHandler::SubmitJob(const JobOutput& output) {
  JobOutput* obj = new JobOutput;
  obj->CopyFrom(output);
  queue_.Put(obj);
}

void SubmitCrawlResultHandler::SubmitStatus(const url::CrawledUrlStatus& status) {
  url::CrawledUrlStatus* obj = new url::CrawledUrlStatus;
  obj->CopyFrom(status);
  status_queue_.Put(obj);
}

}  // namespace crawlwer3
