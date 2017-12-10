#include "spider/crawler/job_manager.h"

#include "base/common/closure.h"
#include "base/thread/thread_util.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/file/file_util.h"
#include "rpc/http_counter_export/export.h"
#include "spider/crawler/kafka/kafka_reader.h"
#include "spider/crawler/kafka/kafka_writer.h"
#include "web/url/url.h"

DEFINE_int64_counter(job_manager, total_job_fetched, 0, "从任务队列当中取出的任务");
DEFINE_int64_counter(job_manager, total_invalid_jobs, 0, "无效的任务数量");
DEFINE_int64_counter(job_manager, total_tried_jobs, 0, "尝试抓取的任务");
DEFINE_int64_counter(job_manager, total_dropped_jobs, 0, "被丢弃的任务");
DEFINE_int64_counter(job_manager, total_job_put_back, 0, "由于某些原因重新返回的任务");
DEFINE_int64_counter(job_manager, total_job_put_back_fail, 0, "返回的任务失败数");

DEFINE_int64_counter(job_manager, total_job_dedup, 0, "所有被排重掉的job的个数");
DEFINE_int64_counter(job_manager, dedup_cache_size, 0, "排重需要的cache的大小");
DEFINE_int64_counter(job_manager, dedup_cache_expire_cnt, 0, "因为排重缓存过期所以没被排重掉的job的个数");

namespace spider {

JobManager::JobManager(const Options &options)
    : stopped_read_(false),
      stopped_write_back_(false),
      options_(options),
      task_submit_(0),
      num_to_fetch_(0),
      url_dedup_cache_(NULL),
      task_done_(0),
      task_release_(0),
      task_invalid_(0),
      task_delete_failed_(0),
      task_delete_success_(0) {
  CHECK_GT(options_.reserve_job_timeout, 0);
  CHECK(options_.kafka_brokers.length() > 0);
  CHECK(options_.kafka_job_request_topic.length() > 0);
  CHECK(options_.kafka_consumer_group_name.length() > 0);

  if (options_.max_dedup_cache_size > 0 && options_.min_dedup_interval_ms > 0) {
    url_dedup_cache_ = new base::LRUCache<std::string, int64>(options_.max_dedup_cache_size, true);
    LOG(INFO) << "Job Manager use url dedup cache, size: " << options_.max_dedup_cache_size << ", min interval ms: "
              << options_.min_dedup_interval_ms;
  } else {
    LOG(INFO) << "Job Manager disable url dedup cache, because size: " << options_.max_dedup_cache_size
              << ", min interval ms: " << options_.min_dedup_interval_ms;
  }
}

JobManager::~JobManager() {
  if (!zk_client_) {
    delete zk_client_;
    zk_client_ = NULL;
  }
  if (!stopped_read_) {
    StopReadDataFromMQ();
  }
  if (!stopped_write_back_) {
    StopWriteBackFailedJobToMQ();
  }
  if (url_dedup_cache_ != NULL) {
    delete url_dedup_cache_;
  }
  if (kafka_reader_ != NULL) {
    delete kafka_reader_;
    kafka_reader_ = NULL;
  }
}

void JobManager::Init() {
  // 启动任务获取线程
  stopped_read_ = false;
  stopped_write_back_ = false;
  job_fetch_thread_.Start(NewCallback(this, &JobManager::CheckJobQueue));
  job_put_thread_.Start(NewCallback(this, &JobManager::PutJobProc));
  return;
}

void JobManager::StopReadDataFromMQ() {
  if (kafka_reader_ != NULL) {
    kafka_reader_->SetStopped();
  }
  LOG(INFO) << "[Stop] start to wait job_fetch_thread_ exit";
  job_fetch_thread_.Join();
  LOG(INFO) << "[Stop] job_fetch_thread_ exit successfully";
  job_fetched_.Close();
  stopped_read_ = true;
}

void JobManager::StopWriteBackFailedJobToMQ() {
  jobs_to_put_.Close();
  while (jobs_to_put_.Size() > 0) {
    LOG(INFO) << "wait the jobs_to_put_ queue to empty, current size is " << jobs_to_put_.Size();
    base::SleepForSeconds(1);
  }
  LOG(INFO) << "[Stop] start to wait job_put_thread_ exit";
  job_put_thread_.Join();
  LOG(INFO) << "[Stop] job_put_thread_ exit successfully";
  stopped_write_back_ = true;
}

void JobManager::TakeAll(JobManager::JobQueue* jobs) {
  CHECK_NOTNULL(jobs);
  job_fetched_.TimedMultiTake(50, jobs);
}

void JobManager::UpdateFetchNum(int num) {
  num_to_fetch_ = num;
}

uint32_t JobManager::GetPendingJobCnt() {
  return job_fetched_.Size();
}

// 一般只有失败次数过多丢弃的, 才会进入此线程
void JobManager::PutJobProc() {
  thread::SetCurrentThreadName("JobManager[PutJobProc]");

  scoped_ptr<KafkaWriter> kafka_writer(new KafkaWriter(options_.kafka_brokers));
  if (!kafka_writer->Init()) {
    LOG(FATAL) << "Failed to init kafka producer: " << options_.kafka_brokers;
  }

  int64 totoal_put_back = 0;
  std::string serialized;
  GeneralCrawler::JobData* job = NULL;
  while ((job = jobs_to_put_.Take()) != NULL) {
    scoped_ptr<GeneralCrawler::JobData> auto_delete(job);
    // 超过最大尝试次数, 直接丢弃
    // 修改重试次数为 1, 即: 抓取一次失败后, 就扔掉. 因为重试次数过多, 会导致队列堆积,影响其他 url
    // 的收录时效性
    // 0903: 不重试时, 抓取成功率降低 7% 左右, 所以跳回 2
    if (job->job.has_tried_times() && job->job.tried_times() >= this->options_.max_retry_times) {
      COUNTERS_job_manager__total_dropped_jobs.Increase(1);
      LOG(ERROR) << "drop url:\t" <<  job->job.detail().url()
                 << "\tretry_times:" << job->job.tried_times();
      continue;
    }

    serialized.clear();
    job->job.set_tried_times(job->job.tried_times() + 1);

    // 策略如下: 如果以前是用 代理抓的, 改用 直接抓取;
    // 反之, 则用代理抓取
    if (!job->job.detail().has_use_proxy() || job->job.detail().use_proxy() == false) {
      job->job.mutable_detail()->set_use_proxy(true);
    } else {
      // 如果透传了代理，判断一下是否还有可用代理
      if (job->job.detail().proxy().empty()) {
        // 没有预设的可用代理了，就设为直连
        job->job.mutable_detail()->set_use_proxy(false);
      } else {
        // 还有客户端透传上的可用代理，这里就不改变代理使用情况了
      }
    }

    LOG_EVERY_N(INFO, 1000) << "[summary] total put back: " << totoal_put_back;
    if (!job->job.SerializeToString(&serialized)) {
      LOG(ERROR) << "Failed to Serialize JobInput to string.";
      continue;
    }

    // 这里是重抓，重抓我们用原来的partition就行
    bool succ = kafka_writer->SendWithPartition(options_.kafka_job_request_topic, job->job.detail().url(), serialized, job->from_partition);
    if (!succ) {
      LOG(ERROR) << "Failed in kafka writer send, url: " << job->job.detail().url();
      COUNTERS_job_manager__total_job_put_back_fail.Increase(1);
    } else {
      totoal_put_back++;
      COUNTERS_job_manager__total_job_put_back.Increase(1);
      LOG(ERROR) << "BACK to kafka request topic, url: " << job->job.detail().url();
    }
  }
}

bool JobManager::isBlackSite(const std::string &url) {
  if (this->options_.black_sites.empty()) {
    return false;
  }
  for (auto it = this->options_.black_sites.begin(); it != this->options_.black_sites.end(); ++it) {
    if (!it->empty() && url.find(*it) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void JobManager::node_data_cb(const std::string &path, const std::string &data) {
  // 只监听自己的consumer
  if (!base::EndsWith(options_.dynamic_conf_addresses, path, true)) {
    return;
  }
  if (!base::StartsWith(data, "range", false) && !base::StartsWith(data, "roundrobin", false)) {
    LOG(ERROR) << "Found invalid assignor: " << data << " in zk path: " << path;
    return;
  }

  std::string first_assignor = "range";
  if (base::StartsWith(data, "roundrobin-r", false)) {
    first_assignor = "roundrobin-r";
  } else if (base::StartsWith(data, "range-r", false)) {
    first_assignor = "range-r";
  } else if (base::StartsWith(data, "roundrobin", false)) {
    first_assignor = "roundrobin";
  }

  if (options_.kafka_assignor != first_assignor) {
    LOG(INFO) << "Found new assignor conf: " << first_assignor << " for consumer: " << path;
    options_.kafka_assignor = first_assignor;
  }
}

void JobManager::CheckJobQueue() {
  thread::SetCurrentThreadName("JM[CheckJobQueue]");
  if (!options_.dynamic_conf_addresses.empty()) {
    size_t pos = options_.dynamic_conf_addresses.find('/');
    std::string zk_host = options_.dynamic_conf_addresses.substr(0, pos);
    std::string zk_path = options_.dynamic_conf_addresses.substr(pos);
    zk_client_ = new ZKClient(zk_host, 3000);
    if (!zk_client_->Init()) {
      LOG(FATAL) << "Failed to connect to zk[" << zk_host << "]";
    }
    if (!zk_client_->CreatePersistentNodeIfPresent(zk_path, options_.kafka_assignor)) {
      LOG(FATAL) << "Failed to init to zk[" << zk_host << "] path [" << zk_path << "]";
    }
    std::string data;
    if (!zk_client_->GetAndWatchNodeData(zk_path, this, &data)) {
      LOG(FATAL) << "Failed to add watcher to zk[" << zk_host << "] path [" << zk_path << "]";
    }
    if (!data.empty()) {
      options_.kafka_assignor = data;
      LOG(INFO) << "Init assignor: " << data << " from zk";
    }
  }

  kafka_reader_ =
      new KafkaReader(options_.kafka_brokers,
                      options_.kafka_job_request_topic,
                      options_.kafka_consumer_group_name,
                      options_.kafka_assignor,
                      options_.kafka_seek_end);
  if (!kafka_reader_->Init()) {
    LOG(FATAL) << "Failed to connect to kafka[" << options_.kafka_brokers
               << ":" << options_.kafka_job_request_topic << "]";
  }

  int task_count = 0;
  while (!kafka_reader_->IsStopped()) {
    // 只有当 当前任务队列小于需要抓取的最大数量 才进行抓取
    while (!kafka_reader_->IsStopped() && job_fetched_.Size() < num_to_fetch_) {
      if (kafka_reader_->GetAssignor() != options_.kafka_assignor) {
        LOG(INFO) << "Kafka assignor is changed from " << kafka_reader_->GetAssignor() << " to "
                  << options_.kafka_assignor << ", so re-open kafka consumer with new assignor";
        delete kafka_reader_;
        kafka_reader_ =
            new KafkaReader(options_.kafka_brokers,
                            options_.kafka_job_request_topic,
                            options_.kafka_consumer_group_name,
                            options_.kafka_assignor,
                            options_.kafka_seek_end);
        if (!kafka_reader_->Init()) {
          LOG(FATAL) << "Failed to connect to kafka[" << options_.kafka_brokers
                     << ":" << options_.kafka_job_request_topic << "]";
        }
        LOG(INFO) << "Succ re-open kafka consumer with assignor: " << kafka_reader_->GetAssignor() << ", consumer: "
                  << options_.kafka_consumer_group_name;
      }

      // 尝试获取任务
      RdKafka::Message *message = kafka_reader_->Fetch();
      if (message == NULL) {
        LOG(WARNING) << "Can't get new job from kafka, because kafka read is closed";
        // Fetch设计永远不返回NULL，如果返回了NULL，说明已经关闭了，程序在退出，如果这里break出去
        break;
      }
      COUNTERS_job_manager__total_job_fetched.Increase(1);

      // 创建任务对应数据并将任务解析处理保存于此对象当中
      task_count++;
      GeneralCrawler::JobData* data = new GeneralCrawler::JobData;
      data->from_partition = message->partition();
      if (!data->job.ParseFromArray(message->payload(), message->len())
          || data->job.detail().url().empty()) {
        LOG(ERROR) << "Failed to parse job, from " << message->topic_name() << "-" << message->partition() << ":"
                   << message->offset() + ", key: " << (message->key() == NULL ? "NULL" : *(message->key()));
        task_invalid_++;
        delete data;
        delete message;
        COUNTERS_job_manager__total_invalid_jobs.Increase(1);
        continue;
      } else if (isBlackSite(data->job.detail().url())) {
        LOG(ERROR) << "Skip black site: " << data->job.detail().url();
        task_invalid_++;
        delete data;
        delete message;
        COUNTERS_job_manager__total_invalid_jobs.Increase(1);
        continue;
      }
      // 切换成kafka后，没有beanstalk的job_id了，这里用partition和offset综合算出一个唯一id
      int64_t job_id = (((uint64_t)message->partition() << 56) | message->offset());

      // 删掉无用的message对象
      delete message;

      // 判断任务是否合法如果不合法，删除任务并限制一段任务文本
      GURL gurl(data->job.detail().url());
      if (!gurl.is_valid()) {
        LOG(ERROR) << "invalid url: "
                   << data->job.detail().url().substr(0, 200);
        task_invalid_++;
        delete data;
        COUNTERS_job_manager__total_invalid_jobs.Increase(1);
        continue;
      }

      // 判断是否需要做排重处理
      if (url_dedup_cache_ != NULL) {
        // 先更新一下计数器，用来上报amonitor
        COUNTERS_job_manager__dedup_cache_size.Reset(url_dedup_cache_->size());
        std::string dedup_key;
        int feaNum = data->job.detail().features_size();
        for (int i = 0; i < feaNum; ++i) {
          const FeatureItem &fea = data->job.detail().features(i);
          const std::string &key = fea.key();
          const std::string &value = fea.value();
          if (key == "dedup_key" && !value.empty()) {
            dedup_key = value;
            break;
          }
        }
        if (dedup_key.empty()) {
          dedup_key = data->job.detail().url();
        }

        // 将重试的次数加进去，人家重试的时候，不能随便去重掉啊
        int try_times = data->job.tried_times();
        dedup_key = base::IntToString(try_times) + "_" + dedup_key;

        int64_t last_schedule_time_ms = 0;
        if (url_dedup_cache_->Get(dedup_key, &last_schedule_time_ms)) {
          if ((base::GetTimestamp() / 1000) < last_schedule_time_ms + options_.min_dedup_interval_ms) {
            // 如果还达到最小抓取间隔, 则直接过滤
            COUNTERS_job_manager__total_job_dedup.Increase(1);
            LOG(INFO) << "dedup url: " << dedup_key << ", last schedule time: " << last_schedule_time_ms
                      << ", min interval: " << options_.min_dedup_interval_ms;
            delete data;
            continue;
          } else {
            // 如果到了最小间隔，那么正常抓取，顺便更新一下新的时间值
            url_dedup_cache_->Add(dedup_key, base::GetTimestamp() / 1000);
            COUNTERS_job_manager__dedup_cache_expire_cnt.Increase(1);
          }
        } else {
          // 如果cache里不存在，说明是最近第一次抓取，放入cache，并记录它第一次被调度的时间
          url_dedup_cache_->Add(dedup_key, base::GetTimestamp() / 1000);
        }
      }

      // 将任务放置到抓取队列当中
      data->jobid = job_id;
      data->done = ::NewCallback(this, &JobManager::JobDone, job_id, data);
      data->timestamp = ::base::GetTimestamp();
      job_fetched_.Put(data);

      std::string proxy_info = data->job.detail().proxy();

      // 打印日志, 表明该 task 已经进入抓取系统, 便于 case 追查
      LOG(INFO) << "fetched from kafka, url: " << data->job.detail().url() << ", use_proxy: "
                << data->job.detail().use_proxy() << ", proxy: " << proxy_info + ", retried: "
                << data->job.tried_times();

      COUNTERS_job_manager__total_tried_jobs.Increase(1);
      LOG_EVERY_N(INFO, 100000) << "[summary] total_task fetched: " << task_count;
      continue;
    }

    if (!kafka_reader_->IsStopped()) {
      // 如果队列已经满了， 需要休息一段时间
      // 选项当中保存的是毫秒数量， 调用 usleep 时需要乘 1000
      base::SleepForSeconds(options_.holdon_when_jobfull);
      LOG_EVERY_N(INFO, 1000) << "when queue is full, sleep for "
                              << options_.holdon_when_jobfull << " seconds, current num_to_fetch_=" << num_to_fetch_;
    }
  }
  // 在这个线程里退出吧
  if (kafka_reader_ != NULL) {
    delete kafka_reader_;
    kafka_reader_ = NULL;
  }
  LOG(INFO) << "Thread JM[CheckJobQueue] exit successfully";
}

void JobManager::JobDone(int64 jobid, GeneralCrawler::JobData* job) {
  // 虽然 status ==0, 也有可能 超时和或者 503 等没有真正抓取成功 
  // 在 fetch result handler 需要进行更细致的处理
  if (job->status == 0) {
    task_done_++;
    delete job;
  } else {
    // 当 url 无效(空窜或者 gurl invalid时, 设置为 -1);
    // 当 一个 url 抓取失败次数过多时, 也会设置为 -1);
    task_release_++;
    jobs_to_put_.Put(job);
  }

  LOG_EVERY_N(INFO, 10000) << "[summary] job completed: " << task_done_
                           << ", job release: " << task_release_
                           << ", job invalid: " << task_invalid_
                           << ", job deleted: " << task_delete_success_
                           << ", job failed to delete: " << task_delete_failed_;
}
}  // namespace spider 
