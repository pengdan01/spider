#include "spider/crawler/fetch_result_handler.h"

#include <deque>

#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/thread/thread_util.h"
#include "base/hash_function/url.h"
#include "base/time/time.h"
#include "base/time/timestamp.h"
#include "rpc/http_counter_export/export.h"
#include "base/strings/utf_codepage_conversions.h"
#include "spider/crawler/kafka/kafka_writer.h"

namespace spider {

DEFINE_int32(submit_process_num, 4, "提交线程个数, 由于需要进行 utf8 转码,所以多线程");
DEFINE_bool(dump_output_switch, false, "Dump the JobOutput For Debug");

DEFINE_int64_counter(resource_handler, total_dropped_docs, 0, "由于队列过长丢弃的任务");
DEFINE_int64_counter(resource_handler, total_job_put_back, 0, "由于某些原因重新返回的任务");
DEFINE_int64_counter(resource_handler, total_job_put_back_fail, 0, "返回的任务失败数");
DEFINE_int64_counter(resource_handler, total_send_result, 0, "向下游kafka发送的结果数");
DEFINE_int64_counter(resource_handler, total_send_result_fail, 0, "向下游kafka发送的结果，失败数");
DEFINE_int64_counter(resource_handler, total_convert_utf8, 0, "转utf8成功");
DEFINE_int64_counter(resource_handler, total_convert_utf8_fail, 0, "转utf8成功失败");

SubmitCrawlResultHandler::SubmitCrawlResultHandler(const Options &options)
    : status_saver_(options.status_prefix, options.saver_timespan), options_(options), stopped_(false) {
  CHECK(options_.kafka_brokers.length() != 0);
  CHECK(options_.kafka_job_request_topic.length() != 0);
  CHECK(options_.kafka_job_result_topic.length() != 0);
}

SubmitCrawlResultHandler::~SubmitCrawlResultHandler() {
  if (!stopped_) {
    Stop();
  }
  if (submit_threads_ != NULL) delete submit_threads_;
  if (kafka_writer_ != NULL) delete kafka_writer_;
}

void SubmitCrawlResultHandler::Init() {
  total_send_ = 0;
  kafka_writer_ = new KafkaWriter(options_.kafka_brokers);
  if (!kafka_writer_->Init()) {
    LOG(FATAL) << "Failed to init kafka producer: " << options_.kafka_brokers;
  }
  submit_threads_ = new thread::ThreadPool(FLAGS_submit_process_num);
  for (int i = 0; i < FLAGS_submit_process_num; ++i) {
    submit_threads_->AddTask(NewCallback(this, &SubmitCrawlResultHandler::SubmitProc));
  }
}

void SubmitCrawlResultHandler::Stop() {
  while (queue_.Size() > 0) {
    LOG(INFO) << "wait the result_html_queue queue to empty, current size is " << queue_.Size();
    base::SleepForSeconds(1);
  }
  stopped_ = true;
  queue_.Close();
  LOG(INFO) << "[Stop] start to wait submit_threads_ exit";
  submit_threads_->JoinAll();
  LOG(INFO) << "[Stop] submit_threads_ exit successfully";
}

bool SubmitCrawlResultHandler::IsNeedRetry(JobOutput *output) {
  CHECK_NOTNULL(output);

  const std::string &url = output->job().detail().url();

  // 1. 超过最大允许重试次数, 不需要重试
  int retryTimes = output->job().tried_times();
  if (retryTimes >= this->options_.max_retry_times) {
    LOG(ERROR) << "Exceed max allowd retry times, url: " << url << ", tried times: " << retryTimes;
    return false;
  }

  // 2. 抓取成功 且 返回值为 200|404, 不需要重试
  if (output->has_res()) {
    int response_code = output->res().brief().crawl_info().response_code();
    if (response_code == 200 || response_code == 404) {
      return false;
    }
  }

  return true;
}

void SubmitCrawlResultHandler::Process(crawler2::crawl::CrawlTask* task,
                                       crawler2::crawl::CrawlRoutineStatus* status,
                                       crawler2::Resource* res, Closure* done) {
  // GeneralCrawler::FetchTaskDone
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

  // 如果需要重试, 就放回队列, 直接 continue
  if (IsNeedRetry(&output)) {
    JobInput *retryJob = new JobInput(output.job());
    PutJobBackProc(retryJob, extra->from_partition);
    return;
  }

  std::string formatted_time;
  base::Time t = ::base::Time::FromDoubleT(
      output.job().submitted_timestamp() / 1000.0 / 1000.0);
  if (output.job().submitted_timestamp() == 0
      || !t.ToStringInFormat("%Y-%m-%d_%H:%M:%S", &formatted_time)) {
    formatted_time = "no time info";
  }
  int64 ts = base::GetTimestamp();
  std::string str = base::StringPrintf(
      "%s\t%s\t%d\t%li\t%s\t%s\t%s\t[%s]\t%li\t%s\t%d",
      task->url.c_str(), (status->success ? "SUCCESS" : "FAILED"),
      status->ret_code, ts,
      status->error_desc.c_str(), (task->proxy.empty() ? "D" : "P"),
      task->proxy.c_str(), extra->handle_key.c_str(),
      extra->jobid, formatted_time.c_str(), output.job().tried_times());
  status_saver_.AppendLine(str, base::GetTimestamp());

  // XXX(pengdan): extra 这个内存已经释放(抓取内部通过 done 回调)
  LOG(INFO) << "url: " << task->url.c_str() << ", ret code: " << status->ret_code;

  if (FLAGS_dump_output_switch) {
    LOG(INFO) << "Output: " << output.DebugString();
  }

  SubmitJob(output);
}

// 将需要重试抓取的 url 放回抓取 kafka
void SubmitCrawlResultHandler::PutJobBackProc(JobInput *job, int32_t from_partition) {
  std::string serialized;
  if (job == NULL) {
    return;
  }
  scoped_ptr<JobInput> auto_job(job);
  job->set_tried_times(job->tried_times() + 1);

  // 策略如下: 如果以前是用 代理抓的, 改用 直接抓取;
  // 反之, 则用代理抓取
  if (!job->detail().has_use_proxy() || job->detail().use_proxy() == false) {
    job->mutable_detail()->set_use_proxy(true);
  } else {
    // 如果透传了代理，判断一下是否还有可用代理
    if (job->detail().proxy().empty()) {
      // 没有预设的可用代理了，就设为直连
      job->mutable_detail()->set_use_proxy(false);
    } else {
      // 还有客户端透传上的可用代理，这里就不改变代理使用情况了
    }
  }

  serialized.clear();
  if (!job->SerializeToString(&serialized)) {
    LOG(ERROR) << "Failed to Serialize JobInput to string.";
    return;
  }

  bool succ = kafka_writer_->SendWithPartition(options_.kafka_job_request_topic, job->detail().url(), serialized, from_partition);
  if (!succ) {
    LOG(ERROR) << "Failed in PutJobBackProc(), url: " << job->detail().url();
    COUNTERS_resource_handler__total_job_put_back_fail.Increase(1);
  } else {
    LOG(ERROR) << "Add back to kafka request topic, url: " << job->detail().url()
               << ", tried times: " << job->tried_times();
    COUNTERS_resource_handler__total_job_put_back.Increase(1);
  }
}

bool SubmitCrawlResultHandler::ConvertHtmlToUtf8(JobOutput *output) {
  CHECK_NOTNULL(output);
  if (output->job().detail().resource_type() != crawler2::kHTML) {
    return false;
  }
  if (!output->has_res()) {
    LOG(ERROR) << "Has no res, url: " << output->job().detail().url();
    return false;
  }
  int response_code = output->res().brief().crawl_info().response_code();
  const std::string &url = output->job().detail().url();
  if (response_code != 200) {
    LOG(ERROR) << "crawl fail, http response code: " << response_code << ", url: " << url;
    return false;
  }

  const std::string &eurl = output->res().brief().effective_url();
  const std::string &raw_content = output->res().content().raw_content();
  if (raw_content.size() == 0u) {
    LOG(ERROR) << "raw content is empty, url: " << url;
    return false;
  }
  int skipped_bytes, nonascii_bytes;
  std::string utf8_html;
  const char *code_page = base::ConvertHTMLToUTF8WithBestEffort(
      eurl, output->res().brief().response_header(), raw_content,
      &utf8_html, &skipped_bytes, &nonascii_bytes);
  if (code_page == NULL || skipped_bytes > (int)raw_content.size() / 10) {
    // 转码失败, 进行后续的失败处理
    LOG(ERROR) << "fail to convert to utf8, url: " << url
               << ", invalid bytes: " << skipped_bytes
               << ", total bytes: " << raw_content.size();
    COUNTERS_resource_handler__total_convert_utf8_fail.Increase(1);
    return false;
  } else {
    // 转码成功! 丢弃 raw_content, 设置 utf8 content
    output->mutable_res()->mutable_content()->set_utf8_content(utf8_html);
    output->mutable_res()->mutable_content()->clear_raw_content();
    LOG(INFO) << "content utf8 convert ok, url: " << url;
    COUNTERS_resource_handler__total_convert_utf8.Increase(1);
    return true;
  }
}

void SubmitCrawlResultHandler::SubmitProc() {
  thread::SetCurrentThreadName("ResultHandler[SubmitProc]");
  int ret = 0;
  JobOutput* obj = NULL;
  while ((ret = queue_.TimedTake(500, &obj)) != -1) {
    LOG_EVERY_N(INFO, 500) << " total task send to downstream is: "
                           << total_send_<< ", current data in queue: "
                           << queue_.Size();
    // 如果超时
    if (ret == 0) continue;
    if (obj == NULL) continue;
    int queue_size = queue_.Size();
    if (queue_size > 100000) {
      std::deque<JobOutput*> deq;
      queue_.TryMultiTake(&deq);
      COUNTERS_resource_handler__total_dropped_docs.Increase(deq.size());
      while (!deq.empty()) {
        LOG(INFO) << "droped url: " << deq.front()->job().detail().url();
        delete deq.front();
        deq.pop_front();
      }
      LOG(ERROR) << "Too many items in queue[" << queue_size << "], drop them directly";
      continue;
    }

    // convert to raw_content to utf8 if crawl succ
    // 不管是否转 utf8 成功, 都需要发给下游, 所以 这里不对返回值进行检测
    scoped_ptr<JobOutput> auto_clear(obj);
    ConvertHtmlToUtf8(obj);

    std::string output;
    if (!obj->SerializeToString(&output)) {
      LOG(ERROR) << "Failed to serialize: " << obj->DebugString() << ", url is: "
                 << obj->job().detail().url();
      continue;
    }

    int priority = 5;
    if (obj->job().detail().has_priority()) {
      priority = obj->job().detail().priority();
    }
    bool succ = kafka_writer_->Send(options_.kafka_job_result_topic, obj->job().detail().url(), output, priority);
    if (!succ) {
      LOG(ERROR) << "Failed to Put task and ignore"
                 << ", url: " << obj->job().detail().url();
      COUNTERS_resource_handler__total_send_result_fail.Increase(1);
      continue;
    }
    COUNTERS_resource_handler__total_send_result.Increase(1);
    total_send_++;
    // 打印提交给下游 job queue 的 url, 便于追查 case
    LOG(INFO) << "added job result to kafka[" << options_.kafka_job_result_topic << "], url: " << obj->job().detail().url();
  }
}

void SubmitCrawlResultHandler::SubmitJob(const JobOutput& output) {
  JobOutput* obj = new JobOutput(output);
  //obj->CopyFrom(output);
  queue_.Put(obj);
}

}  // namespace crawlwer3
