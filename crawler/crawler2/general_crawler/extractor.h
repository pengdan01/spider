#pragma once
#include <cstdatomic>

#include <string>
#include <vector>
#include <unordered_set>

#include "base/thread/thread.h"
#include "base/thread/thread_pool.h"
#include "base/thread/thread_util.h"
#include "base/thread/blocking_queue.h"
#include "base/common/closure.h"
#include "extend/config/config.h"
#include "feature/web/page/html_features.pb.h"
#include "crawler/crawl/time_split_saver.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"

#include "web/url_norm/url_norm.h"
#include "web/url_category/url_categorizer.h"
#include "crawler2/general_crawler/util/url_extract_rule.h"
#include "crawler2/general_crawler/url.pb.h"
#include "crawler2/price_recg/price_recg.h"

namespace redis {
class RedisController;
}

namespace crawler3 {

DECLARE_bool(is_submit_newlink);
DECLARE_int32(max_url_depth);
DECLARE_string(url_extract_pattern_file);
DECLARE_string(recg_data_path);

class GeneralExtractor {
 public:
  struct Options {
    std::string resource_prefix;
    std::string title_prefix;
    int64 saver_timespan;  // 微秒
    int64 res_saver_timespan;  // 微秒
    int processor_thread_num;  // 分析网页的线程数量

    std::string job_queue_ip;
    int job_queue_port;
    std::string job_queue_tube;

    std::string error_titles;

    std::string url_queue_ip;
    int url_queue_port;
    std::string url_queue_tube;

    // 下游输出队列
    std::string res_output_queue_ip;
    int res_output_queue_port;
    std::string res_output_queue_tube;

    Options()
        : job_queue_port(0), url_queue_port(0), res_output_queue_port(0) {
    }
  };

  GeneralExtractor(const Options& options, redis::RedisController& redis_controler);
  ~GeneralExtractor();

  void Start();
  void Stop();
  void Loop();

  int ResourceQueueSize();
 private:
  // 此线程负责分析网页
  void ExtractorProc();

  // 此线程负责不断从任务队列取任务
  void FetchJobProc();

  // 此线程负责提交新 url 到待处理队列
  void SubmitNewUrlProc();

  // 此函数负责将 res 转换成和大爬虫相同的 存储格式
  // 1. bin 格式: 和大爬虫一致
  // 2. text 格式: key 和 value 使用 base64 编码
  void SaveResource(const crawler2::Resource &res);

  // 此函数负责给查询 redis 内存词典, 补全 res 如下信息: uv rank, query, anchor
  bool AddResourceExtraInfo(crawler2::Resource *res, std::string *desc);
  //
  // 此函数负责提交新 url 到本地 blocking queue
  void AddNewUrlToLocalQueue(const crawler2::crawl_queue::JobOutput *output);

  void SaveTaobaoItem(const std::vector< ::crawler3::url::TaobaoProductInfo> &items);
  void AddNewUrls(const std::vector<std::string> &urls);

  // 处理识别京东等价格图片
  bool RecgImagePrice(const std::string &url, const std::string &image_data, std::string *result);
  // 处理苏宁价格数据
  bool ExtractSuNingPrice(const std::string &json_data, std::string *result);
  // 此线程负责将已经完成处理的 res 存放本地 或者提交给下游
  void SubmitResProc();

  std::vector<thread::Thread*> processor_threads_;
  thread::Thread fetch_job_thread_;
  thread::Thread submit_newurl_thread_;
  thread::Thread submit_res_thread_;
  thread::BlockingQueue<crawler2::crawl_queue::JobOutput*> resources_;
  // 存放已经完成处理的, 需要将 res 提交给下游 queue 或者 存到本地的 res
  thread::BlockingQueue<crawler2::crawl_queue::JobOutput*> resources2_;
  thread::BlockingQueue<crawler3::url::NewUrl*> newurls_;
  Options options_;
  bool stopped_;

  crawler2::crawl::TimeSplitSaver resource_saver_;
  crawler2::crawl::TimeSplitSaver title_saver_;
  crawler2::crawl::TimeSplitSaver taobao_saver_;

 public:
  // 统计数据
  std::atomic<int64> total_no_res_;   // 取到的任务当中不包含 res 的数量
 private:
  web::UrlNorm url_norm_;
  web::util::UrlCategorizer url_categoryizer_;
  redis::RedisController& redis_controler_;
  std::unordered_set<std::string> bad_titles_;
  thread::Mutex mutex_;
  // Lock used when writing resource to local file
  thread::Mutex res_mutex_;
  // Lock used when writing title to local file
  thread::Mutex title_mutex_;
  // Lock used when writing taobao items to local file
  thread::Mutex taobao_mutex_;
  // url extract pattern rules
  std::vector<util::RulePattern> extract_patterns_;
  // 京东价格图片识别对象
  crawler2::AsciiRecognize recg_;
};

void ParseGeneralExtractorConfig(const extend::config::ConfigObject& config,
                                  GeneralExtractor::Options* options);
}  // namespace crawler3
