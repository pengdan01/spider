#include <fstream>

#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/closure.h"
#include "base/time/time.h"
#include "base/file/file_util.h"
#include "base/zip/snappy.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/thread/thread_pool.h"
#include "base/thread/blocking_queue.h"
#include "base/thread/thread_util.h"
#include "base/thread/sync.h"
#include "base/hash_function/url.h"
#include "base/encoding/line_escape.h"
#include "base/container/lru_cache.h"

#include "rpc/http_counter_export/export.h"
#include "web/html/api/html_parser.h"
#include "web/html/utils/url_extractor.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"

#include "crawler/realtime_crawler/css_job_manager.h"
#include "crawler/realtime_crawler/css_fetch_result_handler.h"
#include "crawler/fetcher2/fetcher.h"
#include "crawler/fetcher2/data_io.h"

DEFINE_string(page_analyzer_queue_ip, "180.153.227.166", "输出 job queue ip");
DEFINE_int32(page_analyzer_queue_port, 20081, "输出 job queue port");
DEFINE_string(in_queue_ip, "180.153.227.167", "读入 job queue ip");
DEFINE_int32(in_queue_port, 20071, "读入 job queue port");

DEFINE_string(user_agent, "360Spider", "as the name");
DEFINE_int32(max_redir_times, 32, "HTTP 3XX redir times allowed when fetching url");
DEFINE_string(dns_servers, "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25",
              "list of dns servers, seperated by ',' ");

DEFINE_int32(default_max_qps, 3, "max qps per ip by default");
DEFINE_int32(default_max_connections, 6, "max concurrent connections per ip by default");
DEFINE_int32(css_max_connections_in_all, 1800, "css specific max concurrent connections in all");
DEFINE_int32(min_fetch_times, 15, "number of urls fetched before checking qps load each time. 必须大于 1.");
DEFINE_int32(max_holdon_duration_after_failed, 1000 * 10,
             "number mili-seconds to hold on, if failed fecth on one ip.");
DEFINE_int32(min_holdon_duration_after_failed, 1000 * 10,
             "number mili-seconds to hold on, if failed fecth on one ip.");
DEFINE_int32(failed_times_threadhold, 10, "");

DEFINE_string(ip_load_file, "", "the file of ip load, contains 4 fields: "
              "ip \t max_conn \t max_qps \t HH:MM-HH:MM (begin_time-end-time)");
DEFINE_string(proxy_list_file, "", "the file of proxy, each line is a proxy, e.g.: socks5://localhost:2000");
DEFINE_string(https_urls_filename, "", "");

DEFINE_int32(css_fetcher_thread_num, 13, "the number of threads to fetch css");

DEFINE_int32(max_failed_times, 100, "在某 IP 上连续失败超过此限制之后，将丢弃所有此 IP 上的 URL");

DEFINE_int32(proxy_max_successive_failures, 100,
             "通过一个代理抓取连续失败超限, 代理会被判为失效不再使用; "
             "超过一半的代理失效, 程序会崩溃, 需要人工介入检查代理设置.");
DEFINE_int32(max_fetch_times_to_reset_cookie, 35, "对一个 domain 每抓取一定次数, 就清空一下 cookie");

DEFINE_string(css_cache_server_list_file, "",
              "css 缓存服务器列表文件; 缓存 css 的服务器列表, 每行一个服务器, "
              "包括服务器编号，及服务器地址, 用 TAB 分隔; 如果为空, 则不抓取 css");

DEFINE_int32(transfer_timeout_in_seconds, 150, "");
DEFINE_bool(enable_curl_debug, false, "show debug message when fetching url or not");
DEFINE_int32(max_css_cache_size, 2000000, "");
DEFINE_int32(skip_css_queue_size, 100000, "待抓 css 队列长度超过时， 不抓 css 了");

typedef thread::BlockingQueue<crawler2::crawl_queue::JobOutput*> ResourceQueue;
typedef base::LRUCache<std::string, std::string> LRUCssCache;

class FetchedCssCallback: public crawler2::Fetcher::FetchedUrlCallback {
 public:
  explicit FetchedCssCallback(thread::Mutex *mutex, LRUCssCache *css_files) {
    mutex_ = mutex;
    css_files_ = css_files;
  }
  virtual ~FetchedCssCallback() {
    // 不要删除 css_files_, 由外部负责
  }
  // fetcher 每次抓完一个 url, 会调用一次 FetchedUrlCallback::Process() 方法.
  virtual void Process(crawler2::UrlFetched *url_fetched) {
    if (url_fetched->success && url_fetched->res.brief().crawl_info().response_code() == 200) {
      CHECK(url_fetched->res.has_content());
      CHECK(url_fetched->res.content().has_raw_content());
      // XXX: 这里 key 一定要用 url_fetched->url, 不然可能会拼接不上
      {
        thread::AutoLock auto_lock(mutex_);
        css_files_->Add(url_fetched->url_to_fetch.url, url_fetched->res.content().raw_content());
      }
    } else {
      // 外部在抓取之前, 已经负责把 css_files_ 相应项的值设为 "",
      // 这样, 抓取失败的 css 内容就自动为 "", 防止重复抓取无效 css
    }
    LOG(INFO) << "fetched css: " << url_fetched->url_to_fetch.url << ", ret code: " << url_fetched->ret_code;
    delete url_fetched;
  }

 private:
  thread::Mutex *mutex_;
  LRUCssCache *css_files_;
};

// 集中收集所有网页, 插入 css
class ResourceProcessor {
 public:
  ResourceProcessor(
      const crawler2::Fetcher::Options *fetcher_options,
      const crawler2::LoadController::Options *controller_option,
      ResourceQueue *resource_queue,
      crawler2::crawl_queue::SubmitResWithCssHandler *submiter,
      thread::Mutex *mutex,
      LRUCssCache *cache) {
    fetcher_options_ = *fetcher_options;
    controller_option_ = *controller_option;
    queue_ = resource_queue;
    submiter_ = submiter;
    mutex_ = mutex;
    css_files_ = cache;
  }
  ~ResourceProcessor() {}

  void ExtractCssUrls(crawler2::Resource *res) {
    CHECK_NOTNULL(res);
    // 1. 检查 res 各个必要字段是否存在
    if (!res->has_brief()) {
      LOG(ERROR) << "res has no brief";
      return;
    }
    if (!res->has_content() || !res->content().has_raw_content()) {
      LOG(ERROR) << "res has no content or raw_content";
      return;
    }
    if (!res->brief().has_resource_type()
        || res->brief().resource_type() != crawler2::kHTML) {
      LOG(ERROR) << "res has no resource_type or not crawler2::kHTML";
      return;
    }
    if (!res->brief().has_crawl_info()) {
      LOG(ERROR) << "res has no crawl_info";
      return;
    }
    if (!res->brief().crawl_info().has_response_code()
        || res->brief().crawl_info().response_code() != 200) {
      LOG(ERROR) << "res http response code(not 200): " << res->brief().crawl_info().response_code();
      return;
    }
    if (!res->brief().has_effective_url() || !res->brief().has_response_header()) {
      LOG(ERROR) << "res has no effective_url or response_header, url: " << res->brief().source_url();
      return;
    }
    // 2. raw content to utf8
    std::string utf8_html;
    const char *code_page = base::ConvertHTMLToUTF8(
        res->brief().effective_url(), res->brief().response_header(), res->content().raw_content(),
        &utf8_html);
    if (code_page == NULL) {
      LOG(WARNING) << "fail to convert to utf8: " << res->brief().source_url();
    } else {
      // 转码成功, 分析网页, 提取 css/image 链接
      LOG(INFO) << "start to parse html: " << res->brief().source_url();
      std::string effective_url_clicked;
      if (!web::RawToClickUrl(res->brief().effective_url(),
                              NULL, &effective_url_clicked)) {
        LOG(ERROR) << "RawToClick() fail , url: " << res->brief().effective_url()
                   << ". Will NOT extract css/image/title";
      } else {
        // 1. 这里网页的 url 需要用 effective url 而不是 source url.
        // 2. 如果 utf8 网页大于 1MB, 则截断后再处理, 降低网页处理出错的概率
        if (utf8_html.size() > 1024*1024LU) {
          utf8_html.resize(1024*1024LU);
        }
        web::HTMLDoc doc(utf8_html.c_str(), effective_url_clicked.c_str());
        web::HTMLParser parser;
        parser.Init();
        parser.Parse(&doc, web::HTMLParser::kParseAll);  // 无须判断返回值
        {  // css
          std::set<std::string> css_urls;
          for (auto it = doc.css_links().begin(); it != doc.css_links().end(); ++it) {
            if (css_urls.find(*it) != css_urls.end()) continue;
            css_urls.insert(*it);
            res->mutable_parsed_data()->add_css_urls(*it);
          }
        }
        {  // image
          std::set<std::string> image_urls;
          for (auto it = doc.image_links().begin(); it != doc.image_links().end(); ++it) {
            if (image_urls.find(it->url) != image_urls.end()) continue;
            image_urls.insert(it->url);
            res->mutable_parsed_data()->add_image_urls(it->url);
          }
        }
        {  // anchor_url
          std::set<std::string> anchor_urls;
          for (auto it = doc.anchors().begin(); it != doc.anchors().end(); ++it) {
            if (anchor_urls.find(it->first) != anchor_urls.end()) continue;
            anchor_urls.insert(it->first);
            res->mutable_parsed_data()->add_anchor_urls(it->first);
          }
        }
        {  // title
          std::string raw_title = doc.tree()->GetTitle();
          VLOG(5) << "title: " << raw_title << ", url: " << res->brief().source_url();
          if (raw_title.size() > 1024u) {
            raw_title.resize(1024);
          }
          std::string le_title;
          base::LineEscape(raw_title, &le_title);
          res->mutable_content()->set_html_title(le_title);
        }
      }  // end of RawToClick 成功
    }  // end of 转码成功
  }

  void GetUrlsToFetch(const std::deque<crawler2::crawl_queue::JobOutput*> &job_output_list,
                      std::deque<crawler2::UrlToFetch> *css_urls_to_fetch,
                      int *num) {
    CHECK_NOTNULL(css_urls_to_fetch);
    CHECK_NOTNULL(num);
    for (auto it = job_output_list.begin(); it != job_output_list.end(); ++it) {
      if (!(*it)->has_res()) continue;
      if (!(*it)->res().has_brief()) continue;
      if (!(*it)->res().brief().has_crawl_info()) continue;
      if (!(*it)->res().brief().crawl_info().has_response_code()) continue;
      if ((*it)->res().brief().crawl_info().response_code() != 200) continue;
      if (!(*it)->res().has_parsed_data()) continue;

      for (auto css_url_it = (*it)->res().parsed_data().css_urls().begin();
           css_url_it != (*it)->res().parsed_data().css_urls().end(); ++css_url_it) {
        ++(*num);
        thread::AutoLock auto_lock(mutex_);

        bool ret = css_files_->Exists(*css_url_it);
        if (ret == false) {
          (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
          (*css_urls_to_fetch).back().url = (*css_url_it);
          // 伪造 ip, 抓 css 时, 不做压力控制
          (*css_urls_to_fetch).back().ip = (*css_url_it);
          (*css_urls_to_fetch).back().resource_type = crawler2::kCSS;
          (*css_urls_to_fetch).back().importance = 1;
          (*css_urls_to_fetch).back().referer = (*it)->res().brief().effective_url();
          (*css_urls_to_fetch).back().use_proxy = false;
          (*css_urls_to_fetch).back().tried_times = 0;
          // 先占个位置, 避免重复抓取
          css_files_->Add(*css_url_it, "");
        }
      }
    }
  }

  void Loop() {
    thread::SetCurrentThreadName("css_process");
    {
      // 获取队列中的所有数据, 抓取 网页的 CSS, 并插入网页
      std::deque<crawler2::crawl_queue::JobOutput*> job_output_list;
      while (queue_->MultiTake(&job_output_list) != -1) {
        // 队列长度超过 1w ，跳过抓取 css
        if (queue_->Size() > FLAGS_skip_css_queue_size) {
          LOG(INFO) << "skip crawler css, skip num:" << job_output_list.size();
          for (auto it = job_output_list.begin(); it != job_output_list.end(); ++it) {
            // 不管抓取 css 是否成功, 都需要提交
            submiter_->SubmitJob(**it);
            // 需要释放内存, 否则内存泄漏
            delete (*it);
          }
          continue;
        }
        LOG(INFO) << "queue size:" << queue_->Size();

        // 1. 分析 html 文档, 提取出 css urls
        for (auto it = job_output_list.begin(); it != job_output_list.end(); ++it) {
          if (!(*it)->has_res()) continue;
          ExtractCssUrls((*it)->mutable_res());
        }
        // 2. 去重, 选出需要抓取的 css urls
        std::deque<crawler2::UrlToFetch> css_to_fetch;
        int css_num = 0;
        GetUrlsToFetch(job_output_list, &css_to_fetch, &css_num);
        LOG(INFO) << "a batch resouce found, " << job_output_list.size()
                  << " resources, total "  << css_num << " css extracted, "
                  << css_to_fetch.size() << " css to fetch.";

        // 3. 并发抓取这批网页需要的 css
        if (!css_to_fetch.empty()) {
          crawler2::LoadController load_controller(controller_option_);
          CHECK(load_controller.Load(FLAGS_ip_load_file));
          crawler2::Fetcher fetcher(fetcher_options_, load_controller);
          FetchedCssCallback callback(mutex_, css_files_);
          fetcher.FetchUrls(&css_to_fetch, &callback);
        }

        // 4. 插入 css
        for (auto it = job_output_list.begin(); it != job_output_list.end(); ++it) {
          crawler2::Resource &res = *((*it)->mutable_res());
          if (res.has_parsed_data() && res.parsed_data().css_urls_size() > 0) {
            for (auto css_url_it = res.parsed_data().css_urls().begin();
                 css_url_it != res.parsed_data().css_urls().end();
                 ++css_url_it) {
              std::string css_content;

              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(*css_url_it, &css_content);

              if (ret == true) {
                if (!css_content.empty()) {
                  auto cit = res.mutable_content()->add_css_files();
                  cit->set_url(*css_url_it);
                  cit->set_raw_content(css_content);
                } else {
                  css_files_->Remove(*css_url_it);
                }
              }
            }
          }
          // 不管抓取 css 是否成功, 都需要提交
          submiter_->SubmitJob(**it);
          // 需要释放内存, 否则内存泄漏
          delete (*it);
        }

        job_output_list.clear();
      }  // end while (queue_->MultiTake(&tmp) ...)
    }
  }  // end Loop(...)

 private:
  crawler2::Fetcher::Options fetcher_options_;
  crawler2::LoadController::Options controller_option_;

  ResourceQueue *queue_;
  crawler2::crawl_queue::SubmitResWithCssHandler *submiter_;

  thread::Mutex *mutex_;
  LRUCssCache *css_files_;

  DISALLOW_COPY_AND_ASSIGN(ResourceProcessor);
};

void LoadCacheServersOrDie(const std::string &filename, std::vector<std::string> *cache_servers) {
  std::vector<std::string> lines;
  CHECK(base::file_util::ReadFileToLines(filename, &lines))
      << "failed to load cache servers from file: " << filename;

  cache_servers->clear();
  for (int i = 0; i < (int)lines.size(); ++i) {
    const std::string &line = lines[i];
    std::vector<std::string> tokens;
    base::SplitString(line, "\t ", &tokens);
    CHECK_EQ(tokens.size(), 2u) << "invalid line in cache server list: " << line;
    int index;
    LOG_IF(FATAL, !base::StringToInt(tokens[0], &index) || index != i)
        << "cache server id should be " << i << ", for line: " << line;
    cache_servers->push_back(tokens[1]);
  }
}

void SetFetchOption(crawler2::Fetcher::Options &options) {
  options.connect_timeout_in_seconds = 16;  // typically, 8
  options.transfer_timeout_in_seconds = FLAGS_transfer_timeout_in_seconds;  // typically, 150
  options.low_speed_limit_in_bytes_per_second = 500;  // typically, 500
  options.low_speed_duration_in_seconds = FLAGS_transfer_timeout_in_seconds;

  options.user_agent = FLAGS_user_agent;
  options.max_redir_times = 30;  // typically, 30
  options.use_proxy_dns = false;  // typically, false
  options.always_create_new_link = false;  // typically, false
  if (!FLAGS_https_urls_filename.empty()) {
    crawler2::LoadHttpsFiles(FLAGS_https_urls_filename, &options.https_urls);
  }
  if (!FLAGS_proxy_list_file.empty()) {
    std::string content;
    CHECK(base::file_util::ReadFileToString(FLAGS_proxy_list_file, &content));
    base::SplitStringWithOptions(content, "\n", true, true, &options.proxy_servers);
  }
  std::vector<std::string> dns_servers;
  base::SplitStringWithOptions(FLAGS_dns_servers, ",", true, true, &dns_servers);
  options.dns_servers = dns_servers;
  options.enable_curl_debug = FLAGS_enable_curl_debug;
  options.max_fetch_num = 0;  // for test only
  options.proxy_max_successive_failures = FLAGS_proxy_max_successive_failures;  // typically, 100
  CHECK_GT(options.proxy_max_successive_failures, 0);
  std::vector<std::string> css_cache_servers;
  //if (!FLAGS_css_cache_server_list_file.empty()) {
  //  LoadCacheServersOrDie(FLAGS_css_cache_server_list_file, &css_cache_servers);
  //  options.css_cache_servers = css_cache_servers;
  //}
}

void SetLoadControlOption(crawler2::LoadController::Options &controller_option) {
  controller_option.default_max_qps = FLAGS_default_max_qps;
  controller_option.default_max_connections = FLAGS_default_max_connections;
  controller_option.max_connections_in_all = FLAGS_css_max_connections_in_all;
  controller_option.max_holdon_duration_after_failed = FLAGS_max_holdon_duration_after_failed;
  controller_option.min_holdon_duration_after_failed = FLAGS_min_holdon_duration_after_failed;
  controller_option.failed_times_threadhold = FLAGS_failed_times_threadhold;
  controller_option.max_failed_times = FLAGS_max_failed_times;
}

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "fetcher");

  LOG(INFO) << "Start HTTPCounter on port: " << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();

  // 1. setup options
  crawler2::Fetcher::Options options;
  SetFetchOption(options);

  crawler2::LoadController::Options controller_option;
  SetLoadControlOption(controller_option);

  // 2. 提交添加 css 后的 res 给下游分析 
  LOG(INFO) << "Css DataHandler starting up...";
  crawler2::crawl_queue::SubmitResWithCssHandler::Options handler_option; 
  handler_option.default_queue_ip = FLAGS_page_analyzer_queue_ip;
  handler_option.default_queue_port = FLAGS_page_analyzer_queue_port;
  crawler2::crawl_queue::SubmitResWithCssHandler handler(handler_option);
  handler.Init();

  // 3. 抓取 css
  ResourceQueue resource_queue;
  thread::ThreadPool resource_pool(FLAGS_css_fetcher_thread_num);
  std::vector<ResourceProcessor *> resource_processors;
  LOG(INFO) << "Start css resource processor, threads num: " << FLAGS_css_fetcher_thread_num;
  // 全局互斥锁
  thread::Mutex global_mutex_lock;
  // 全局 cache 队列
  LRUCssCache global_css_files(FLAGS_max_css_cache_size, false); 
  for (int i = 0; i < FLAGS_css_fetcher_thread_num; ++i) {
    ResourceProcessor *rp = new ResourceProcessor(&options, &controller_option, &resource_queue, &handler, &global_mutex_lock, &global_css_files);
    resource_processors.push_back(rp);
    resource_pool.AddTask(NewCallback(rp, &ResourceProcessor::Loop));
  }

  // 4. 获取输入 
  LOG(INFO) << "Css JobManager starting up...";
  crawler2::crawl_queue::CssJobManager::Options css_job_mgr_option;
  css_job_mgr_option.queue_ip = FLAGS_in_queue_ip;
  css_job_mgr_option.queue_port = FLAGS_in_queue_port;
  crawler2::crawl_queue::CssJobManager css_job_mgr(css_job_mgr_option, resource_queue);
  css_job_mgr.Init();
 
  // 正常情况下, 程序启动后, 就不会退出
  resource_pool.JoinAll();

  return 0;
}
