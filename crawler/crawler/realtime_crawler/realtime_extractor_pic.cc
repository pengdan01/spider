#include "crawler/realtime_crawler/realtime_extractor.h"

#include "web/url/url.h"
#include "crawler/realtime_crawler/cdoc_util.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "base/common/logging.h"
#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_printf.h"
#include "base/time/timestamp.h"
#include "base/hash_function/url.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/job_queue/job_queue.h"
#include "rpc/http_counter_export/export.h"

#include "web/url_category/url_categorizer.h"

namespace crawler2 {
namespace crawl_queue {

DEFINE_int64_counter(realtime_extractor, cdocs_extract, 0, "分析的符合文档数");
DEFINE_int64_counter(realtime_extractor, res_without_content, 0, "不包含内容的resource 数量");
DEFINE_int64_counter(realtime_extractor, queue_length, 0, "本地队列的长度");
DEFINE_int32(html_debug, 0, "0: cdoc 提交到leafserver， 1: cdoc 输出到log(info)");
DEFINE_int32(url_categorizer, 1, "0: 不使用 url_categorizer 过滤 ， 1: 过滤");

RealtimeExtractor::RealtimeExtractor(const Options& options,
                                     IndexRequestSubmitter* index_request_submitter,
                                     IndexRequestSubmitter* index_request_submitter2,
                                     redis::RedisController& redis_controler,
                                     bool mode)
    : options_(options)
      , stopped_(false)
      , index_request_submitter_(index_request_submitter)
      , index_request_submitter2_(index_request_submitter2)
      // XXX(pengdan): res 文件数据量大, 每 300 秒 写一次; 使用 binary 写入
      , resource_saver_(options.resource_prefix, options.res_saver_timespan, true)
      , title_saver_(options.title_prefix, options.saver_timespan)
      , total_fetched_(0)
      , total_cdocs_generated_(0)
      , total_no_res_(0)
      , total_failed_(0)
      , redis_controler_(redis_controler)
      , backup_mode_(mode) {
        CHECK_GT(options.saver_timespan, 0);
        CHECK(!options.resource_prefix.empty());
        CHECK(!options.title_prefix.empty());
        CHECK(!options.job_queue_ip.empty());
        CHECK_NE(options.job_queue_port, 0);
        CHECK(!options.error_titles.empty());
      }

bool RealtimeExtractor::AddResourceExtraInfo(crawler2::Resource &res, std::string *desc) {
  CHECK_NOTNULL(desc);

  // 设置 实时爬虫抓取标志位
  res.mutable_brief()->mutable_crawl_reason()->set_is_realtime_crawl(true);

  // uv rank, query, anchor
  if (!res.has_brief()) {
    *desc = "res has no brief";
    return false;
  }
  if (!res.brief().has_source_url()) {
    *desc = "res has no source url";
    return false;
  }
  if (!res.brief().has_crawl_info()) {
    *desc = "res has no crawl info";
    return false;
  }
  if (!res.brief().crawl_info().has_response_code()) {
    *desc = "res has no response_code";
    return false;
  }
  if (res.brief().crawl_info().response_code() != 200) {
    *desc = "response_code != 200, is: " + base::StringPrintf("%ld",res.brief().crawl_info().response_code());
    return false;
  }

  std::string click_url, normed_source_url;
  if (!::web::RawToClickUrl(res.brief().source_url(), NULL, &click_url)
      || !url_norm_.NormalizeClickUrl(click_url, &normed_source_url)) {
    *desc = "Failed to RawToClick or NormClickUrl, url: " + res.brief().source_url();
    return false;
  }

  uint64 url_sign = ::base::CalcUrlSign(normed_source_url.data(), normed_source_url.size());
  std::vector<uint64> url_signs;
  url_signs.push_back(url_sign);
  std::map<uint64, std::map<std::string, std::string>> redis_results;
  // BatchInquire() 返回 true, 但是不一定找到元素
  if (!redis_controler_.BatchInquire(url_signs, &redis_results)) {
    *desc = "url not found in redis: " + res.brief().source_url() + ", sign: "
        + base::StringPrintf("%lu", url_sign);
    return false;
  }

  auto r_it = redis_results.find(url_sign);
  if (r_it == redis_results.end()) {
    *desc = "url not found in redis: " + res.brief().source_url() + base::StringPrintf("%lu", url_sign);
    return false;
  }

  VLOG(3) << "url found in redis: " << res.brief().source_url() << ", sign: " << url_sign;

  const std::map<std::string, std::string>& field_map = r_it->second;

  // uv rank
  auto f_it = field_map.find("1");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.has_uv_rank());
    VLOG(3) << "uv rank: " << graph.uv_rank().Utf8DebugString();
    res.mutable_graph()->mutable_uv_rank()->Swap(graph.mutable_uv_rank());
  }

  // click rank
  f_it = field_map.find("2");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.has_click_rank());
    VLOG(3) << "click rank: " << graph.click_rank().Utf8DebugString();
    res.mutable_graph()->mutable_click_rank()->Swap(graph.mutable_click_rank());
  }

  // query
  f_it = field_map.find("3");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.query_size() != 0);
    for (int query_idx = 0; query_idx < graph.query_size(); ++query_idx) {
      VLOG(3) << "query[" << query_idx << "]:"
              << graph.query(query_idx).Utf8DebugString();
      graph.mutable_query(query_idx)->Swap(res.mutable_graph()->add_query());
    }
  }

  // anchor
  f_it = field_map.find("4");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.anchor_size() != 0);
    for (int anchor_idx = 0; anchor_idx < graph.anchor_size(); ++anchor_idx) {
      VLOG(3) << "anchor[" << anchor_idx << "]:"
              << graph.anchor(anchor_idx).Utf8DebugString();
      graph.mutable_anchor(anchor_idx)->Swap(res.mutable_graph()->add_anchor());
    }
  }
  *desc = "done, url: " + res.brief().source_url();
  return true;
}

void RealtimeExtractor::ExtractorProc() {
  thread::SetCurrentThreadName("RealtimeExtractor[ExtractorProc]");

  CDocGenerator gen;
  CHECK(gen.Init(options_.error_titles)) << "Failed to Initialize CDocGenerator";
  JobOutput* output = NULL;
  ArchiveData data;
  while ((output = resources_.Take()) != NULL) {
    data.cdoc.clear();
    data.title.clear();
    data.job = output;

    // 对于 effective url 用 UrlCategorizer 进行过滤
    //if (FLAGS_url_categorizer) {
    //  const std::string &e_url = output->res().brief().effective_url();
    //  int url_id;
    //  std::string tmp;
    //  if (!url_categoryizer_.Category(e_url, &url_id, &tmp)) {
    //    LOG(ERROR) << "Failed to call UrlCategorizer::Category on url, e_url: " << e_url << ", source url: "
    //               <<  output->res().brief().source_url();
    //    // XXX(pengdan): 解析失败的, 释放掉内存
    //    delete output;
    //    continue;
    //  }
    //  if (url_id < web::util::kSplitLineForCrawler) {
    //    LOG(ERROR) << "url is droped, e_url: " << e_url << ", it typeid: " << url_id << ", source url: "
    //               << output->res().brief().source_url();
    //    delete output;
    //    continue;
    //  }
    //}

    //std::string result_desc;
    //LOG_IF(WARNING, !AddResourceExtraInfo(*output->mutable_res(), &result_desc))
    //    << "Warning, AddResourceExtraInfo() result: " << result_desc;

    //// 提取复合文档
    //LOG_EVERY_N(INFO, 1000) << "[summary]Total cdocs generated is: " << total_cdocs_generated_;
    //std::string title;
    //feature::HTMLInfo info;
    //// 在生成复合文档时, 已经根据是否是 bad title 进行网页过滤
    //if (!gen.Gen(&info, output->job().detail(), output->mutable_res(),
    //             &title)) {
    //  total_failed_++;
    //  LOG(ERROR) << "Failed to generator cdoc for url:" << output->job().detail().url();
    //  delete output;
    //  continue;
    //}

    //// 保存符合文档以备提交实时索引
    //if (!info.SerializeToString(&(data.cdoc))) {
    //  LOG(ERROR) << "Failed to serialize HTMLInfo: " << info.DebugString();
    //  delete output;
    //  continue;
    //}

    //if (FLAGS_html_debug == 1) {
    //  // LOG(INFO) << info.DebugString();
    //  LOG(INFO) << info.effective_url();
    //  delete output;
    //  continue;
    //}

    //std::string tmp;
    //// XXX(pengdan): 去除 title 两边的空白字符
    //::base::TrimWhitespaces(&title);
    //::base::LineEscape(title, &tmp);
    //data.title = ::base::StringPrintf("%s\t%s\t%ld",
    //                                  info.effective_url().c_str(),
    //                                  tmp.c_str(), ::base::GetTimestamp());
    archives_.Put(data);
    total_cdocs_generated_++;
    COUNTERS_realtime_extractor__cdocs_extract.Increase(1);
    COUNTERS_realtime_extractor__queue_length.Reset(resources_.Size());
    
  }
}

void RealtimeExtractor::FetchJobProc() {
  thread::SetCurrentThreadName("RealtimeExtractor[FetchJobProc]");

  // 负责从 job_queue 当中获取数据并检查数据的合法性
  job_queue::Client client(options_.job_queue_ip, options_.job_queue_port);
  CHECK(client.Connect()) << "Failed to connect to job_queue["
                          << options_.job_queue_ip << ":"
                          << options_.job_queue_port << "]";
  if (!options_.job_queue_tube.empty()) {
    CHECK(client.Watch(options_.job_queue_tube))
        << "Failed to call Watch[" << options_.job_queue_tube <<"]";
  }

  LOG(ERROR) << "Success to connect server["
             << options_.job_queue_ip << ":" << options_.job_queue_port
             << ":" << options_.job_queue_tube << "]";

  int64 task_fetched = 0;
  while (true) {
    if (resources_.Size() > 1000) {
      LOG_EVERY_N(INFO, 1000)
          << " Too many task in queue [" <<  resources_.Size()
          << "] sleep for sometime";
      usleep(100 * 1000);
    }

    if (!client.Ping() && !client.Reconnect()) {
      LOG(ERROR) << "Failed to connect to jobqueue ["
                 << options_.job_queue_ip << ":"
                 << options_.job_queue_port <<"]";
      usleep(100 * 0000);
      continue;
    }

    job_queue::Job job;
    if (!client.ReserveWithTimeout(&job, 1)) {
      LOG_EVERY_N(INFO, 1000) << "[summary] Get Job timeout.";
      continue;
    }

    total_fetched_++;
    if (!client.Delete(job.id)) {
      LOG(ERROR) << "Failed to delete job:" << job.id;
    }

    task_fetched++;
    LOG_EVERY_N(INFO, 1000) << "[summary] tasks[" << task_fetched
                            << "] have been fetched..";

    JobOutput* output = new JobOutput;
    if (!output->ParseFromString(job.body)) {
      LOG(ERROR) << "Failed to parse JobOutput from string."
                 << " job.body.length: " << job.body.length()
                 << " job.body: " << job.body.substr(0, 100);
      delete output;
      continue;
    }

    if (!output->has_res()) {
      total_no_res_++;
      LOG(ERROR) << "Failed, has no res, url: " << output->job().detail().url();
      COUNTERS_realtime_extractor__res_without_content.Increase(1);
      delete output;
      continue;
    }

    resources_.Put(output);
  }
}

int RealtimeExtractor::ResourceQueueSize() {
  return resources_.Size();
}

void RealtimeExtractor::SaveResourceInKeyValueFormat(const crawler2::Resource &res) {
  if (!res.has_brief()) {
    LOG(ERROR) << "has no brief"; 
    return;
  }
  if (!res.brief().has_crawl_info()) {
    LOG(ERROR) << "has no crawl_info"; 
    return;
  }
  if (!res.brief().crawl_info().has_response_code()) {
    LOG(ERROR) << "has no response_code"; 
    return;
  }
  if (res.brief().crawl_info().response_code() != 200) {
    LOG(ERROR) << "url: " << res.brief().source_url() << ", ret code is: "
               << res.brief().crawl_info().response_code(); 
    return;
  }

  std::string key, value; 
  crawler2::GenNewHbaseKey(res.brief().url(), &key);
  ResourceToHbaseDump(res, &value);
  resource_saver_.AppendKeyValue(key, value, ::base::GetTimestamp());
}

void RealtimeExtractor::SubmitProc() {
  thread::SetCurrentThreadName("RealtimeExtractor[SubmitProc]");

  int64 total_cdocs = 0;
  while (true) {
    if (archives_.Closed()) {
      break;
    }

    ArchiveData data  = archives_.Take();
    if (data.job == NULL) {
      LOG(ERROR) << "blocking queue: archieve_ closed.";
      break;
    }

    scoped_ptr<JobOutput> auto_output(data.job);
    // CHECK_NOTNULL(index_request_submitter_);
    // // 将 title 数据保存起来
    // title_saver_.AppendLine(data.title, ::base::GetTimestamp());

    // // 生成的 res
    SaveResourceInKeyValueFormat(data.job->res());

    // 准备提交请求
    // index_request_submitter_->AddCDoc(data.cdoc);
    // if (backup_mode_ == true) {
    //   CHECK_NOTNULL(index_request_submitter2_);
    //   index_request_submitter2_->AddCDoc(data.cdoc);
    // }

    // 打印提交给 index request submitter 的 url, 便于 case 追查
    LOG(INFO) << "saved in local, url: " << data.job->res().brief().source_url();

    total_cdocs++;

    LOG_EVERY_N(INFO, 1000) << "[summary] cdocs[" << total_cdocs
                            << "] has been received.";
  }
}

void RealtimeExtractor::Start() {
  submit_thread_.Start(NewCallback(this, &RealtimeExtractor::SubmitProc));
  fetch_job_thread_.Start(NewCallback(this, &RealtimeExtractor::FetchJobProc));
  for (int i = 0; i < options_.processor_thread_num; ++i) {
    ::thread::Thread *thread = new ::thread::Thread;
    thread->Start(::NewCallback(this, &RealtimeExtractor::ExtractorProc));
    processor_threads_.push_back(thread);
  }
  LOG(ERROR) << "RealtimeExtractor Started.";
}

void RealtimeExtractor::Stop() {
  resources_.Close();
  fetch_job_thread_.Join();
  submit_thread_.Join();
  for (auto iter = processor_threads_.begin();
       iter != processor_threads_.end(); ++iter) {
    (*iter)->Join();
    delete (*iter);
  }

  stopped_ = true;
}

void RealtimeExtractor::Loop() {
  while (!stopped_) {
    sleep(1);
  }
}

void ParseRealtimeExtractorConfig(const extend::config::ConfigObject& config,
                                  RealtimeExtractor::Options* options) {
  options->resource_prefix = config["resource_prefix"].value.string;
  options->title_prefix = config["title_prefix"].value.string;
  options->saver_timespan = config["saver_timespan"].value.integer * 1000 * 1000l;
  options->res_saver_timespan = config["res_saver_timespan"].value.integer * 1000 * 1000l;
  options->processor_thread_num = config["processor_thread_num"].value.integer;
  options->job_queue_ip = config["job_queue_ip"].value.string;
  options->job_queue_port = config["job_queue_port"].value.integer;
  options->job_queue_tube = config["job_queue_tube"].value.string;
  options->error_titles = config["error_titles"].value.string;
}

}  // namespace crawler_queue
}  // namespace feautre
