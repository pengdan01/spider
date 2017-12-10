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
#include "web/html/api/html_node.h"
#include "web/html/api/html_constant.h"
#include "web/html/utils/url_extractor.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"
#include "extend/regexp/re3/re3.h"

#include "crawler2/general_crawler/css_job_manager.h"
#include "crawler2/general_crawler/css_fetch_result_handler.h"
#include "crawler2/general_crawler/util/url_util.h"
#include "crawler/fetcher2/fetcher.h"
#include "crawler/fetcher2/data_io.h"

DEFINE_string(page_analyzer_queue_ip, "180.153.227.166", "输出 job queue ip");
DEFINE_int32(page_analyzer_queue_port, 20081, "输出 job queue port");
DEFINE_string(in_queue_ip, "180.153.227.167", "读入 job queue ip");
DEFINE_int32(in_queue_port, 20071, "读入 job queue port");

DEFINE_string(user_agent, "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)", "as the name");
DEFINE_int32(max_redir_times, 32, "HTTP 3XX redir times allowed when fetching url");
DEFINE_string(dns_servers, "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25",
              "list of dns servers, seperated by ',' ");

DEFINE_int32(default_max_qps, 3, "max qps per ip by default");
DEFINE_int32(default_max_connections, 5, "max concurrent connections per ip by default");
DEFINE_int32(css_max_connections_in_all, 1200, "css specific max concurrent connections in all");
DEFINE_int32(min_fetch_times, 5, "number of urls fetched before checking qps load each time. 必须大于 1.");
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

DEFINE_int32(max_failed_times, 500, "在某 IP 上连续失败超过此限制之后，将丢弃所有此 IP 上的 URL");

DEFINE_int32(proxy_max_successive_failures, 100,
             "通过一个代理抓取连续失败超限, 代理会被判为失效不再使用; "
             "超过一半的代理失效, 程序会崩溃, 需要人工介入检查代理设置.");
// XXX(pengdan): 由于 curl 内部库的问题, 这个选项没有生效.
// 当前的做法是: 有外部脚本定期删除 cookies
DEFINE_int32(max_fetch_times_to_reset_cookie, 35, "对一个 domain 每抓取一定次数, 就清空一下 cookie");

DEFINE_string(css_cache_server_list_file, "",
              "css 缓存服务器列表文件; 缓存 css 的服务器列表, 每行一个服务器, "
              "包括服务器编号，及服务器地址, 用 TAB 分隔; 如果为空, 则不抓取 css");
DEFINE_int32(transfer_timeout_in_seconds, 180, "");
DEFINE_bool(enable_curl_debug, false, "show debug message when fetching url or not");
DEFINE_int32(max_css_cache_size, 20000, "");

DEFINE_bool(crawl_css, true, "是否抓取网页中的 css");
DEFINE_bool(crawl_image, true, "是否抓取网页中的 image");
DEFINE_bool(crawl_other, true, "是否抓取网页中的其他链接, 如评论,商品价格等");

typedef thread::BlockingQueue<crawler2::crawl_queue::JobOutput*> ResourceQueue;
typedef base::LRUCache<std::string, std::string> LRUCssCache;

using extend::re3::Re3;

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
      crawler3::SubmitResWithCssHandler *submiter,
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
      // 转码失败, 进行后续的失败处理
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
        res->mutable_content()->set_utf8_content(utf8_html);
        res->mutable_brief()->set_effective_url(effective_url_clicked);

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

          // NEW
          // XXX(pengdan): 需要了解节点的属性, 例如,对于 <a href=xxx ref=nofollow>anchor_text</a>
          // 应该丢弃这类 urls
          // web::HTMLParser parser;
          // parser.Init();
          // web::HTMLTree tree;
          // CHECK_EQ(parser.ParseString(utf8_html.c_str(), 0, &tree), 0u) << "fail, url: "
          //                                                               << effective_url_clicked;
          // std::set<std::string> anchor_urls;
          // const std::vector<web::HTMLNode*>& nodes = tree.anchor_nodes();
          // LOG(ERROR) << "node size: " << nodes.size();
          // for (auto it = nodes.begin(); it != nodes.end(); ++it) {
          //   const std::string &url= (*it)->AttrValue(web::HTMLAttr::kHref);
          //   if (url.empty()) {
          //     LOG(ERROR) << "href url in node is empty";
          //     continue;
          //   }
          //   const std::string &value = (*it)->AttrValue(web::HTMLAttr::kContent);
          //   const std::string &value2 = (*it)->AttrValue(web::HTMLAttr::kRel);
          //   if (value == "nofollow" || value2 == "nofollow") {
          //     LOG(ERROR) << "ignore nofollow url, url is: " << url << ", parent url is: "
          //                << effective_url_clicked;
          //     continue;
          //   }
          //   if (anchor_urls.find(url) != anchor_urls.end()) continue;
          //   anchor_urls.insert(url);
          //   res->mutable_parsed_data()->add_anchor_urls(url);
          //   LOG(ERROR) << "extract new anchor url, url is: " << url;
          // }
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

  // 添加和京东有关的需要抓取的 url, 如: 价格图片, 评论
  void AddJingDongExtraUrls(const std::string &jingdong_product_url,
                            crawler2::crawl_queue::JobOutput *job_output,
                            std::deque<crawler2::UrlToFetch> *css_urls_to_fetch,
                            int *num) {
    const std::string &url = jingdong_product_url;
    crawler2::Resource &res = *(job_output->mutable_res());

    // 添加 京东价格图片
    std::string price_image_url;
    if (crawler3::BuildJingDongPriceImageLink(url, &price_image_url)) {
      res.mutable_alt()->mutable_price_image()->set_url(price_image_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(price_image_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (price_image_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(price_image_url).host());
        (*css_urls_to_fetch).back().resource_type = crawler2::kImage;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(price_image_url, "");
        ++(*num);
      }
    }
    // 添加 京东用户评论链接
    std::string comment_url;
    if (crawler3::BuildJingDongCommentLink(url, &comment_url)) {
      res.mutable_alt()->mutable_comment_data()->set_url(comment_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(comment_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (comment_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(comment_url).host());
        // Json 数据
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(comment_url, "");
        ++(*num);
      }
    }
  }

  // 添加和苏宁有关的需要抓取的 url, 如: 价格数据, 评论
  void AddSuNingExtraUrls(const std::string &suning_product_url,
                          crawler2::crawl_queue::JobOutput *job_output,
                          std::deque<crawler2::UrlToFetch> *css_urls_to_fetch,
                          int *num) {
    const std::string &url = suning_product_url;
    crawler2::Resource &res = *(job_output->mutable_res());

    // 添加 苏宁价格 url
    std::string price_url;
    if (crawler3::BuildSuNingPriceLink(url, &price_url)) {
      res.mutable_alt()->mutable_price_image()->set_url(price_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(price_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (price_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(price_url).host());
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(price_url, "");
        ++(*num);
      }
    }
    // 添加 苏宁用户评论链接
    std::string comment_url;
    if (crawler3::BuildSuNingCommentLink(url, &comment_url)) {
      res.mutable_alt()->mutable_comment_data()->set_url(comment_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(comment_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (comment_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(comment_url).host());
        // Json 数据
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(comment_url, "");
        ++(*num);
      }
    }
  }

  // 添加和淘宝有关的需要抓取的 url, 如: 评论, 商品详情, 商品参数
  void AddTaobaoExtraUrls(const std::string &taobao_product_url_m,
                          crawler2::crawl_queue::JobOutput *job_output,
                          std::deque<crawler2::UrlToFetch> *css_urls_to_fetch,
                          int *num) {
    const std::string &url = taobao_product_url_m;
    crawler2::Resource &res = *(job_output->mutable_res());

    // 添加 淘宝用户评论链接
    std::string comment_url;
    if (crawler3::BuildTaobaoCommentLink(url, &comment_url)) {
      res.mutable_alt()->mutable_comment_data()->set_url(comment_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(comment_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (comment_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(comment_url).host());
        // Json 数据
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(comment_url, "");
        ++(*num);
      }
    }

    // 添加 淘宝商品描述详情链接
    std::string desc_url;
    if (crawler3::BuildTaobaoProductDetailLink(url, &desc_url)) {
      res.mutable_alt()->mutable_desc_detail()->set_url(desc_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(desc_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (desc_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(desc_url).host());
        // Json 数据
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(desc_url, "");
        ++(*num);
      }
    }

    // 添加 淘宝商品参数
    std::string param_url;
    if (crawler3::BuildTaobaoProductParamLink(url, &param_url)) {
      res.mutable_alt()->mutable_param()->set_url(param_url);

      thread::AutoLock auto_lock(mutex_);
      bool ret = css_files_->Exists(param_url);
      if (ret == false) {
        (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);
        (*css_urls_to_fetch).back().url = (param_url);
        // 伪造 ip, 抓 css 时, 不做压力控制
        (*css_urls_to_fetch).back().ip = (GURL(param_url).host());
        // Json 数据
        (*css_urls_to_fetch).back().resource_type = crawler2::kText;
        (*css_urls_to_fetch).back().importance = 1;
        (*css_urls_to_fetch).back().referer = url;
        (*css_urls_to_fetch).back().use_proxy = false;
        (*css_urls_to_fetch).back().tried_times = 0;
        // 先占个位置, 避免重复抓取
        css_files_->Add(param_url, "");
        ++(*num);
      }
    }
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

      // 添加 css url
      if (FLAGS_crawl_css) {
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

      // 添加图片链接:
      const std::string &host = GURL((*it)->res().brief().source_url()).host();
      if (FLAGS_crawl_image) {  // 设置开关, 是否提取图片链接
        if (host == "a.m.taobao.com" || host == "a.m.tmall.com") {
          for (auto image_url_it = (*it)->res().parsed_data().image_urls().begin();
               image_url_it != (*it)->res().parsed_data().image_urls().end(); ++image_url_it) {
            const std::string &host = GURL(*image_url_it).host();
            // 仅仅抓取商品图片
            if (!base::EndsWith(host, "wimg.taobao.com", false)) {
              continue;
            }
            ++(*num);
            thread::AutoLock auto_lock(mutex_);

            std::string url;
            crawler3::ConvertTaobaoImageUrl(*image_url_it, &url);
            bool ret = css_files_->Exists(url);
            if (ret == false) {
              (*css_urls_to_fetch).resize(css_urls_to_fetch->size() + 1);

              // XXX(pengdan): 转换的目的: 实际抓取的是原始 size 的图片
              (*css_urls_to_fetch).back().url = url;
              // 伪造 ip, 抓 image 时, 不做压力控制
              (*css_urls_to_fetch).back().ip = host;

              (*css_urls_to_fetch).back().resource_type = crawler2::kImage;
              (*css_urls_to_fetch).back().importance = 1;
              (*css_urls_to_fetch).back().referer = (*it)->res().brief().effective_url();
              (*css_urls_to_fetch).back().use_proxy = false;
              (*css_urls_to_fetch).back().tried_times = 0;
              // 先占个位置, 避免重复抓取
              css_files_->Add(url, "");
            }
          }
        }
      }

      // 添加需要额外抓取的重要链接
      const std::string &url = (*it)->res().brief().source_url();
      if (FLAGS_crawl_other) {
        // 京东商品页
        static const Re3 jingdong_product_url_pattern("http://www.360buy.com/product/[0-9]+.html?");
        // 苏宁商品页
        static const Re3 suning_product_url_pattern("http://www.suning.com/emall/prd_[-_0-9]+.html?");
        // 淘宝 天猫 商品页
        static const Re3 m_version_taobao("http://a.m.taobao.com/i(\\d+).html?");
        static const Re3 m_version_tmall("http://a.m.tmall.com/i(\\d+).html?");
        if (Re3::FullMatch(url, jingdong_product_url_pattern)) {
          AddJingDongExtraUrls(url, *it, css_urls_to_fetch, num);
        } else if (Re3::FullMatch(url, suning_product_url_pattern)) {
          AddSuNingExtraUrls(url, *it, css_urls_to_fetch, num);
        } else if (Re3::FullMatch(url, m_version_taobao) ||
                   Re3::FullMatch(url, m_version_tmall)) {
          // 天猫 和 淘宝 (Mobile 版本) 的评论页 构造方式是一样的
          AddTaobaoExtraUrls(url, *it, css_urls_to_fetch, num);
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

        // 4. 插入 css 以及 其他资源, 如 京东图片, 评论等
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

          // 插入 image: 目前之抓取淘宝商品图片
          if (res.has_parsed_data() && res.parsed_data().image_urls_size() > 0) {
            for (auto image_url_it = res.parsed_data().image_urls().begin();
                 image_url_it != res.parsed_data().image_urls().end();
                 ++image_url_it) {
              std::string url;
              crawler3::ConvertTaobaoImageUrl(*image_url_it, &url);
              std::string image_content;
              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(url, &image_content);
              if (ret == true) {
                if (!image_content.empty()) {
                  auto cit = res.mutable_content()->add_image_files();
                  cit->set_url(url);
                  cit->set_raw_content(image_content);
                  DLOG(ERROR) << "image url: " << url << ", done";
                } else {
                  css_files_->Remove(url);
                }
              }
            }
          }

          // 检查是否抓取了特殊图片, 如京东价格图片; 苏宁价格 json 数据
          if (res.has_alt() && res.alt().has_price_image() && res.alt().price_image().has_url()) {
            const std::string &url = res.alt().price_image().url();
            if (!url.empty()) {
              std::string image;
              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(url, &image);
              if (ret == true) {
                if (!image.empty()) {
                  crawler2::ImageFile *image_file = res.mutable_alt()->mutable_price_image();
                  image_file->set_raw_content(image);
                } else {
                  css_files_->Remove(url);
                }
              }
            }
          }
          // 检查是否以及抓取了特殊数据, 如用户评论数据
          if (res.has_alt() && res.alt().has_comment_data() && res.alt().comment_data().has_url()) {
            const std::string &url = res.alt().comment_data().url();
            if (!url.empty()) {
              std::string comment;
              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(url, &comment);
              if (ret == true) {
                if (!comment.empty()) {
                  crawler2::CommentData *comment_data = res.mutable_alt()->mutable_comment_data();
                  comment_data->set_raw_content(comment);
                } else {
                  css_files_->Remove(url);
                }
              }
            }
          }
          // 检查是否以及抓取了特殊数据, 如 淘宝商品描述详情
          if (res.has_alt() && res.alt().has_desc_detail() && res.alt().desc_detail().has_url()) {
            const std::string &url = res.alt().desc_detail().url();
            if (!url.empty()) {
              std::string data;
              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(url, &data);
              if (ret == true) {
                if (!data.empty()) {
                  crawler2::CommentData *desc = res.mutable_alt()->mutable_desc_detail();
                  desc->set_raw_content(data);
                } else {
                  css_files_->Remove(url);
                }
              }
            }
          }
          // 检查是否以及抓取了特殊数据, 如 淘宝商品参数数据
          if (res.has_alt() && res.alt().has_param() && res.alt().param().has_url()) {
            const std::string &url = res.alt().param().url();
            if (!url.empty()) {
              std::string data;
              thread::AutoLock auto_lock(mutex_);
              bool ret = css_files_->Peek(url, &data);
              if (ret == true) {
                if (!data.empty()) {
                  crawler2::CommentData *param = res.mutable_alt()->mutable_param();
                  param->set_raw_content(data);
                } else {
                  css_files_->Remove(url);
                }
              }
            }
          }

          // 不管抓取是否成功, 都需要提交
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
  crawler3::SubmitResWithCssHandler *submiter_;

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

void SetFetchOption(crawler2::Fetcher::Options *options) {
  (*options).connect_timeout_in_seconds = 16;  // typically, 8
  (*options).transfer_timeout_in_seconds = FLAGS_transfer_timeout_in_seconds;  // typically, 150
  (*options).low_speed_limit_in_bytes_per_second = 500;  // typically, 500
  (*options).low_speed_duration_in_seconds = FLAGS_transfer_timeout_in_seconds;

  (*options).direct_user_agent = FLAGS_user_agent;
  (*options).proxy_user_agent = FLAGS_user_agent;
  (*options).max_redir_times = 30;  // typically, 30
  (*options).use_proxy_dns = false;  // typically, false
  (*options).always_create_new_link = false;  // typically, false
  if (!FLAGS_https_urls_filename.empty()) {
    crawler2::LoadHttpsFiles(FLAGS_https_urls_filename, &options->https_urls);
  }
  if (!FLAGS_proxy_list_file.empty()) {
    std::string content;
    CHECK(base::file_util::ReadFileToString(FLAGS_proxy_list_file, &content));
    base::SplitStringWithOptions(content, "\n", true, true, &options->proxy_servers);
  }
  std::vector<std::string> dns_servers;
  base::SplitStringWithOptions(FLAGS_dns_servers, ",", true, true, &dns_servers);
  (*options).dns_servers = dns_servers;
  (*options).enable_curl_debug = FLAGS_enable_curl_debug;
  (*options).max_fetch_num = 0;  // for test only
  (*options).proxy_max_successive_failures = FLAGS_proxy_max_successive_failures;  // typically, 100
  CHECK_GT(options->proxy_max_successive_failures, 0);
  options->max_fetch_times_to_reset_cookie = FLAGS_max_fetch_times_to_reset_cookie;  // typically, 35
  CHECK_GT(options->max_fetch_times_to_reset_cookie, 0);
  std::vector<std::string> css_cache_servers;
  if (!FLAGS_css_cache_server_list_file.empty()) {
    LoadCacheServersOrDie(FLAGS_css_cache_server_list_file, &css_cache_servers);
    options->css_cache_servers = css_cache_servers;
  }
}

void SetLoadControlOption(crawler2::LoadController::Options *controller_option) {
  controller_option->default_max_qps = FLAGS_default_max_qps;
  controller_option->default_max_connections = FLAGS_default_max_connections;
  controller_option->max_connections_in_all = FLAGS_css_max_connections_in_all;
  controller_option->max_holdon_duration_after_failed = FLAGS_max_holdon_duration_after_failed;
  controller_option->min_holdon_duration_after_failed = FLAGS_min_holdon_duration_after_failed;
  controller_option->failed_times_threadhold = FLAGS_failed_times_threadhold;
  controller_option->max_failed_times = FLAGS_max_failed_times;
}

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "fetcher");

  LOG(INFO) << "Start HTTPCounter on port: " << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();

  // 1. setup options
  crawler2::Fetcher::Options options;
  SetFetchOption(&options);

  crawler2::LoadController::Options controller_option;
  SetLoadControlOption(&controller_option);

  // 2. 提交添加 css 后的 res 给下游分析
  LOG(INFO) << "Css DataHandler starting up...";
  crawler3::SubmitResWithCssHandler::Options handler_option;
  handler_option.default_queue_ip = FLAGS_page_analyzer_queue_ip;
  handler_option.default_queue_port = FLAGS_page_analyzer_queue_port;
  crawler3::SubmitResWithCssHandler handler(handler_option);
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
    ResourceProcessor *rp = new ResourceProcessor(&options, &controller_option, &resource_queue,
                                                  &handler, &global_mutex_lock, &global_css_files);
    resource_processors.push_back(rp);
    resource_pool.AddTask(NewCallback(rp, &ResourceProcessor::Loop));
  }

  // 4. 获取输入
  LOG(INFO) << "Css JobManager starting up...";
  crawler3::CssJobManager::Options css_job_mgr_option;
  css_job_mgr_option.queue_ip = FLAGS_in_queue_ip;
  css_job_mgr_option.queue_port = FLAGS_in_queue_port;
  crawler3::CssJobManager css_job_mgr(css_job_mgr_option, resource_queue);
  css_job_mgr.Init();

  // 正常情况下, 程序启动后, 就不会退出
  resource_pool.JoinAll();

  return 0;
}
