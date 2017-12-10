#pragma once

#include <cstdatomic>
#include <string>
#include <set>
#include <vector>
#include <deque>

#include "base/container/lru_cache.h"
#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "rpc/job_queue/job_queue.h"

#include "spider/crawler/crawler.h"
#include "spider/crawler/kafka/kafka_reader.h"
#include "spider/crawler/zookeeper/zkclient.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace spider {
using crawler2::crawl_queue::JobDetail;

class GeneralCrawler;
class JobDetail;

class JobManager: public NodeDataWatcher {
 public:
  struct Options {
    // job queue 相关属性
    std::string kafka_brokers;
    std::string kafka_job_request_topic;
    std::string kafka_consumer_group_name;
    std::string kafka_assignor;
    bool kafka_seek_end;
    // 动态配置的地址
    std::string dynamic_conf_addresses;

    // 最多尝试次数, 如果超过，job 将被丢弃
    int max_retry_times;
    int reserve_job_timeout;
    int holdon_when_jobfull;  // 单位毫秒

    // 排重使用的cache相关配置
    int max_dedup_cache_size;
    int min_dedup_interval_ms;

    // 站点黑名单
    std::vector<std::string> black_sites;
    Options()
        : kafka_seek_end(false)
        , max_retry_times(3)
        , reserve_job_timeout(600)
        , holdon_when_jobfull(100)
        , max_dedup_cache_size(0)
        , min_dedup_interval_ms(0) {
    }
  };

  explicit JobManager(const Options& options);
  ~JobManager();

  void Init();
  // 停止从消息队列中读数据
  void StopReadDataFromMQ();
  // 停止向消息队列中回写失败数据
  void StopWriteBackFailedJobToMQ();

  typedef std::deque<GeneralCrawler::JobData*> JobQueue;

  // 尝试从队列当中取出所有任务
  void TakeAll(JobQueue* jobs);
  void UpdateFetchNum(int num);
  // 当前已从kafka取出，但还未取走的job个数
  uint32_t GetPendingJobCnt();

  // 监听zookeeper上的数据变量
  void node_data_cb(const std::string &path, const std::string &data);

 private:
  void PutJobProc();
  void CheckJobQueue();
  bool isBlackSite(const std::string& url);

  void JobDone(int64 jobid, GeneralCrawler::JobData* job);

  // 从 job queue 中获取任务
  thread::Thread job_fetch_thread_;
  // 从需要重试的任务(如压力控制而没有被调度抓取) 回灌到 job queue
  thread::Thread job_put_thread_;
  // 存放需要回灌任务的 blocking queue
  thread::BlockingQueue<GeneralCrawler::JobData*> jobs_to_put_;
  // 存放从 job queue 中获取到的任务的 blocking queue
  thread::BlockingQueue<GeneralCrawler::JobData*> job_fetched_;

  std::atomic<bool> stopped_read_;
  std::atomic<bool> stopped_write_back_;
  Options options_;
  int task_submit_;
  int num_to_fetch_;

  // url排重用的cache
  base::LRUCache<std::string, int64>* url_dedup_cache_;
  // kafka读取器
  KafkaReader* kafka_reader_;
  // zookeeper监听器
  ZKClient* zk_client_;
 public:
  std::atomic<int64> task_done_;
  std::atomic<int64> task_release_;
  std::atomic<int64> task_invalid_;
  std::atomic<int64> task_delete_failed_;
  std::atomic<int64> task_delete_success_;
};
}  // namespace spider 
