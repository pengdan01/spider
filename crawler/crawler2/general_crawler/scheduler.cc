#include "crawler2/general_crawler/scheduler.h"

#include <map>
#include <string>

#include "base/thread/thread_util.h"
#include "log_analysis/page_importance/index_model/index_model.h"

namespace crawler3 {

static double kConnectionFailClientMaxCount = 3;

DEFINE_string(target_ip, "", "指的是抓取完该 url 后, 需要将该 url 提交到那个 队列的 ip");
DEFINE_int32(target_port, 5000, "指的是抓取完该 url 后, 需要将该 url 提交到那个 队列的 port");
DEFINE_string(target_tube, "", "指的是抓取完该 url 后, 需要将该 url 提交到那个 队列的管道名");
DEFINE_int32(target_priority, 5, "指的是抓取完该 url 后, 需要将该 url 提交任务优先级");

DEFINE_string(index_model_bin_file_path, "", "the binary data file path of index model");
DEFINE_double(index_model_score_threshold, 0.01, "urls with score less then |index_model_score_threshold| "
              "will not be crawled");

// 1 * 24 * 3600 = 86400
DEFINE_int64(max_duration, 86400, "当前距离上一次抓取的 最长时间间隔(seconds), "
             "如果一个 url 的抓取时间间隔超过 |max_duration|, 即使这个 url 在库中, 也会重新抓取");

DEFINE_int32(thread_num, 10, "调度处理线程数目");
DEFINE_int32(max_buffer_size, 100000, "为了去重, 本地由一个 buffer 存储已经处理过的 urls, "
             "|max_buffer_size| 指的是 buffer 记录数的最大值");

DEFINE_int64_counter(scheduler, job_reserved, 0, "从任务队列当中取出的任务");
DEFINE_int64_counter(scheduler, invalid_jobs, 0, "无效的任务数or urls number");
DEFINE_int64_counter(scheduler, failed_put_jobs, 0, "the number of jobs failed to put to crawl queue");
DEFINE_int64_counter(scheduler, submit_jobs, 0, "the number of jobs submit to crawl queue");
DEFINE_int64_counter(scheduler, failed_connect_times, 0, "failed times of doing connection");

DEFINE_int32(dispatch_strategy, 0, "0: 根据 ip 或者 GURL(url).host() 签名分发; 1: 随机分发");

Scheduler::Scheduler(const Options &in_options,
                     const std::vector<Options> &out_options,
                     redis::RedisController &controler)
    : redis_controler_(controler),
    in_options_(in_options),
    out_options_(out_options) {
      last_clear_ts_ = base::GetTimestamp();
      stop_ = false;
      pr_ = new base::PseudoRandom(base::GetTimestamp());
    }

Scheduler::~Scheduler() {
  Stop();
}

bool Scheduler::Init() {
  // 1. 构造 job queue Client
  // CHECK(BuildConnection());
  return true;
}

bool Scheduler::CheckConnection(job_queue::Client *client, int type, const Options &options) {
  if (!client->IsConnected()) {
    if (!client->Connect()) {
      LOG(ERROR) << "Failed to connect to queue["
                 << options.queue_ip << ":" << options.queue_port <<"]";
      return false;
    }
  }

  if (!client->Ping()) {
    if (!client->Reconnect()) {
      LOG(ERROR) << "Failed to connection jobqueue["
                 << options.queue_ip << ":" << options.queue_port <<"]";
      return false;
    }
  }

  // 判断是否需要从 queue 当中获取
  if (!options.queue_tube.empty()) {
    bool ret;
    if (type == 0) {
      ret = client->Watch(options.queue_tube);
    } else {
      ret = client->Use(options.queue_tube);
    }
    if (!ret) {
      LOG(ERROR) << "Failed to call Watch/Use(" << options.queue_tube
                 <<") on queue[" << options.queue_ip << ":"
                 << options.queue_port <<"]";
      return false;
    }
  }

  return true;
}

bool Scheduler::BuildConnection(job_queue::Client **r_client, std::vector<job_queue::Client*> *w_clients) {
  // delete the old client  if any
  if (*r_client) delete *r_client;
  if (w_clients->size() != 0U) {
    for (int i = 0; i < (int)w_clients->size(); ++i) {
      if ((*w_clients)[i]) delete (*w_clients)[i];
    }
    w_clients->clear();
  }

  job_queue::Client *read_client = new job_queue::Client(in_options_.queue_ip, in_options_.queue_port);
  if (!CheckConnection(read_client, 0, in_options_)) {
    LOG(ERROR) << "connect fail[" << in_options_.queue_ip<< ":"
               << in_options_.queue_port << "]";
    COUNTERS_scheduler__failed_connect_times.Increase(1);
    delete read_client;
    return false;
  } else {
    LOG(ERROR) << "Success connect to job queue for read[" << in_options_.queue_ip<< ":"
               << in_options_.queue_port << "]";
  }

  std::vector<job_queue::Client*> write_clients;
  for (int i = 0; i < (int)out_options_.size(); ++i) {
    job_queue::Client *write_client = new job_queue::Client(out_options_[i].queue_ip, out_options_[i].queue_port);
    // 由于存在众多 output queue, 个别 queue 连接异常是常态, 不能个别 queue 异常后就退出
    if (!CheckConnection(write_client, 1, out_options_[i])) {
      LOG(ERROR) << "Connect fail, queue[" << out_options_[i].queue_ip<< ":"
                 << out_options_[i].queue_port << "]";
      COUNTERS_scheduler__failed_connect_times.Increase(1);
      delete write_client;
      write_client = NULL;
    } else {
      LOG(ERROR) << "Success connect to job queue[" << out_options_[i].queue_ip<< ":"
                 << out_options_[i].queue_port << "]";
    }
    write_clients.push_back(write_client);
  }

  // 看连接失败的 client 数
  {
    int fail_cnt = 0;
    for (int i = 0; i < (int)write_clients.size(); ++i) {
      if (!write_clients[i]) {
        ++fail_cnt;
        continue;
      }
      w_clients->push_back(write_clients[i]);
    }
    if (fail_cnt >= kConnectionFailClientMaxCount) {
      // 直接 FATAL
      LOG(FATAL) << "Connection failed write-client num: " << fail_cnt
                 << ", exceed max num: " << kConnectionFailClientMaxCount;
    }
    if (w_clients->size() == 0U) {
      // 直接 FATAL
      LOG(FATAL) << "Connection failed write-client num: 0";
    }
  }

  *r_client = read_client;

  return true;
}

bool Scheduler::ValidateUrl(const std::string &url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    LOG(ERROR) << "url invalid(gurl.is_valid() false):  " << url;
    return false;
  }
  if (gurl.scheme() != "http" && gurl.scheme() != "https") {
    LOG(ERROR) << "url scheme must be http or https:  " << url;
    return false;
  }

  int url_id;
  std::string tmp;
  if (!url_categoryizer_.Category(url, &url_id, &tmp)) {
    LOG(ERROR) << "Failed in UrlCategorizer::Category on url["
               << url <<"]";
    return false;
  }

  // 百度 or google 的 结果页面不需要
  // List page is need
  if (url_id < web::util::kSplitLineForShow &&
      url_id != web::util::kListPage &&
      url_id != web::util::kSpecialFile &&
      url_id != web::util::kSkippable) {
    LOG(ERROR) << "URL[" << url << "] was dropped, urltype id: " << url_id;
    return false;
  }

  return true;
}

int32 Scheduler::SetPriority(double score) {
  CHECK_GE(score, 0.0);
  int32 p = 0;
  if (score >= 500.0) {
    p = pr_->GetInt(0, 9);
  } else if (score >= 100.0) {
    p = pr_->GetInt(10, 29);
  } else if (score >= 10.0) {
    p = pr_->GetInt(30, 49);
  } else {
    p = pr_->GetInt(50, 99);
  }
  return p;
}

// TODO(pengdan): |url_status| 构造 request input
void Scheduler::BuildInputRequest(const crawler3::url::NewUrl &newurl,
                                  double v,
                                  crawler3::url::CrawledUrlStatus *url_status,
                                  crawler2::crawl_queue::JobInput *input) {
  CHECK_NOTNULL(input);
  CHECK_GE(v, 0.0);
  input->set_submitted_timestamp(base::GetTimestamp());
  input->mutable_detail()->set_url(newurl.clicked_url());
  input->mutable_detail()->set_depth(newurl.depth());
  input->mutable_detail()->set_resource_type(newurl.resource_type());
  input->mutable_detail()->set_importance(v);
  if (newurl.has_referer() && !newurl.referer().empty()) {
    input->mutable_detail()->set_referer(newurl.referer());
  }
  if (!FLAGS_target_ip.empty()) {
    input->mutable_target_queue()->set_ip(FLAGS_target_ip);
    input->mutable_target_queue()->set_port(FLAGS_target_port);
    input->mutable_target_queue()->set_tube_name(FLAGS_target_tube);
    input->mutable_target_queue()->set_priority(FLAGS_target_priority);
  }
  input->mutable_detail()->set_priority(SetPriority(v));
}

// 根据策略,将 url 下发给某一个 crawler 监控队列
bool Scheduler::DispatchRequest(const crawler2::crawl_queue::JobInput &input,
                                const std::vector<job_queue::Client *> &write_clients) {
  int client_size = write_clients.size();
  CHECK_GT(client_size, 0);
  const std::string &url = input.detail().url();
  const std::string &ip  = input.detail().ip();
  // 选取一个 crawler 监控队列
  int w_client_index = 0;
  if (FLAGS_dispatch_strategy == 0) {
    std::string hash_input_str;
    if (!ip.empty()) {
      hash_input_str = ip;
    } else {
      hash_input_str = GURL(url).host();
    }
    uint64 hash = base::CityHash64(hash_input_str.data(), hash_input_str.size());
    // XXX(pengdan): Hard Coding
    w_client_index = (hash + pr_->GetInt(0, 2)) % client_size;
  } else if (FLAGS_dispatch_strategy == 1) {
    w_client_index = pr_->GetInt(0, client_size-1);
  } else {
    LOG(FATAL) << "invalid dispatch strategy value: " << FLAGS_dispatch_strategy;
  }

  // 序列化输出
  std::string output;
  CHECK(input.SerializeToString(&output)) << "input.SerializeToString() fail, url: " << url;
  int priority = input.detail().priority();
  
  // 尝试写入队列
  // 当一个队列写入失败, 会依次尝试 |kMaxTryTimes-1| 次重写写入
  int try_cnt = 0;
  const int kMaxTryTimes = 3;
  while (try_cnt < kMaxTryTimes) { 
    job_queue::Client *write_client = write_clients[w_client_index];
    if (write_client != NULL) {
      if (0 >= write_client->PutEx(output.c_str(), output.length(), priority, 0, 10)) {
        LOG(ERROR) << "write_client->PutEx() fail, queue[" << write_client->GetServerIp()
                   << ":" << write_client->GetServerPort() << "]";
        COUNTERS_scheduler__failed_put_jobs.Increase(1);
      } else {
        LOG(ERROR) << "add to job queue[" << write_client->GetServerIp() << ":"
                   << write_client->GetServerPort() << "], url: " << url;
        break;
      }
    }
    ++try_cnt;
    ++w_client_index;
  }
  // 尝试 |kMaxTryTimes| 次写入都失败, 怎么办?
  if (try_cnt == kMaxTryTimes) {
    LOG(ERROR) << "write url to queue: tried kMaxTryTimes(3) times and fail";
    return false;
  }
  return true;
}

bool Scheduler::GetUrlStatus(const std::string &url, crawler3::url::CrawledUrlStatus *url_status) {
  CHECK_NOTNULL(url_status);

  uint64 sign = base::CalcUrlSign(url.c_str(), url.size());
  std::vector<uint64> signs;
  signs.push_back(sign);
  std::map<uint64, std::map<std::string, std::string>> output;
  if (!redis_controler_.BatchInquire(signs, &output)) {
    LOG(ERROR) << "redis_controler_.BatchInquery() return false";
    return false;
  }
  const std::map<std::string, std::string> & field_value = output[sign];
  auto it = field_value.find(crawler3::kCrawlUrlStatusField);
  if (it == field_value.end()) {
    LOG(INFO) << "not find field: " << crawler3::kCrawlUrlStatusField;
    return false;
  }
  DLOG(INFO) << "find url in redis, url is: " << url;

  if (!url_status->ParseFromString(it->second)) {
    LOG(ERROR) << "url_status->ParseFromString() fail, input str: " << it->second;
    return false;
  }

  return true;
}

void Scheduler::Process() {
  thread::SetCurrentThreadName("Scheduler[Process]");

  // Load Index Model
  index_model::ModelPredict *model_predict = NULL;
  if (!FLAGS_index_model_bin_file_path.empty()) {
    model_predict = new index_model::ModelPredict();
    model_predict->LoadModel(FLAGS_index_model_bin_file_path);
  }

  job_queue::Client *read_client = NULL;
  std::vector<job_queue::Client *> write_clients;
  while (!stop_) {
    // 0. 建立链接
    // read queue 建立连接失败时, 函数返回 false
    // write queue 失败占比超过 30%时, 直接 FATAL
    if (!BuildConnection(&read_client, &write_clients)) {
      LOG(ERROR)<< "Connect fail(read queue fail), after 3s retry";
      sleep(3);
      continue;
    }
    while (true) {
      if (stop_) break; 
      job_queue::Job job;
      // 1. 从 待处理队列中获取 url
      if (!read_client->ReserveWithTimeout(&job, 10)) {
        LOG_EVERY_N(INFO, 1000) << "Get Job timeout.";
        if (!CheckConnection(read_client, 0, in_options_)) {
          // 如果链接状态不可用，则直接跳出循环，等待下一轮
          LOG(ERROR) << "Failed to check connection";
          COUNTERS_scheduler__failed_connect_times.Increase(1);
          break;
        }
        continue;
      }
      COUNTERS_scheduler__job_reserved.Increase(1);
      if (!read_client->Delete(job.id)) {
        LOG(ERROR) << "Failed to delete job: " << job.id;
        continue;
      }

      // 避免本地 buffer 过大, 超过大小后, 清除
      check_and_clear_local_set(FLAGS_max_buffer_size, FLAGS_max_duration);

      crawler3::url::NewUrl newurl;
      if (!newurl.ParseFromString(job.body) || newurl.clicked_url().empty()) {
        LOG(ERROR) << "Failed to parse job: " << job.body.substr(0, 200);
        COUNTERS_scheduler__invalid_jobs.Increase(1);
        continue;
      }
      const std::string &clicked_url = newurl.clicked_url();

      // 2. 本地 url 判重, 如果不存在,则先添加到本地缓存 set, 避免重复处理
      if (is_exist_in_local_set(clicked_url)) {
        LOG(INFO) << "Exist in local buffer and ignored, url: " << clicked_url;
        continue;
      }
      add_to_local_set(clicked_url);

      // 3. url 有效性判断
      if (!ValidateUrl(clicked_url)) {
        LOG(ERROR) << "ignore url: " << clicked_url;
        COUNTERS_scheduler__invalid_jobs.Increase(1);
        continue;
      }
      // 4. 查询 redis 全局判重(对重复的定义: 出现且在更新期以内)
      crawler3::url::CrawledUrlStatus url_status;
      bool ret = GetUrlStatus(clicked_url, &url_status);
      if (ret == true) {
        int64 now = base::GetTimestamp();
        int64 latest_crawl_ts = url_status.latest_crawl_timestamp();
        int64 duration_in_secs = (now - latest_crawl_ts) / 1000 / 1000;
        // 超过抓取间隔上限, 则再次抓取
        if (url_status.is_crawled() && duration_in_secs < FLAGS_max_duration) {
          LOG(INFO) << "has crawled recently and ignore url: " << clicked_url;
          continue;
        }
      }
      // 5. Calc score using Index Model
      double v = 0.5;
      bool calc_ok = false;
      if (model_predict != NULL) {
        calc_ok = model_predict->Score(clicked_url, &v);
        if (!calc_ok)  {
          LOG(ERROR) << "Calc score using index model fail, url is: " << clicked_url;
        } else {
          LOG(INFO) << "index model score: " << v << ", url is: " << clicked_url;
        }
      }
      if (calc_ok && v < FLAGS_index_model_score_threshold) {
        LOG(INFO) << "index model score[" << v << "] < index_model_score_threshodl["
                  << FLAGS_index_model_score_threshold << "] and ignored, url: " << clicked_url;
        continue;
      }
      // 6. 构造抓取请求
      crawler2::crawl_queue::JobInput input;
      if (ret == true) {
        BuildInputRequest(newurl, v, &url_status, &input);
      } else {
        BuildInputRequest(newurl, v, NULL, &input);
      }
      // 7. 转发任务给下游
      if (!DispatchRequest(input, write_clients)) {
        LOG(ERROR) << "Dispatch url fail , url: " << input.detail().url();
        break;
      }

      COUNTERS_scheduler__submit_jobs.Increase(1);
    }
  }

  if (model_predict != NULL) {
    delete model_predict;
  }
  if (read_client != NULL) {
    delete read_client;
  }
  for (auto it = write_clients.begin(); it != write_clients.end(); ++it) {
    if (*it != NULL) delete *it;
  }

  return;
}

void Scheduler::Start() {
  int thread_num = FLAGS_thread_num;
  for (int i = 0; i < thread_num; ++i) {
    thread::Thread *thread = new thread::Thread();
    thread->Start(NewCallback(this, &Scheduler::Process));
    threads_.push_back(thread);
  }
}

void Scheduler::Stop() {
  stop_ = true;
  for (auto it = threads_.begin(); it != threads_.end(); ++it) {
    (*it)->Join();
    delete (*it);
  }
  delete pr_;
}

}  // namespace crawler
