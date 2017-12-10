#include "crawler/fetcher/multi_fetcher.h"

#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <curl/multi.h>

#include <iostream>
#include <cmath>
#include <sstream>
#include <fstream>
#include <list>
#include <map>
#include <algorithm>

#include "base/common/logging.h"
#include "base/time/timestamp.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_printf.h"
#include "base/hash_function/fast_hash.h"
#include "base/file/file_util.h"
#include "web/url/url.h"
#include "crawler/util/command.h"
#include "crawler/selector/crawler_selector_util.h"

// TODO(pengdan): 库 的 gflags 需要 加上 模块作为 gflags 前缀
// 是否抓取 整个图片, 默认情况下只需要获取图片的 size 信息
DEFINE_bool(switch_crawl_whole_image, false, "if set, will crawl the whole image");
// 是否使用 过滤规则 对抓取的 url 进行过滤
DEFINE_bool(switch_filter_rules, true, "switch of using crawler filter rules");
// 一个 http  会话的最长持续时间
DEFINE_int32(max_http_session_time, 200, "");
// 单节点对站点的最大并发访问数
DEFINE_int32(max_concurrent_site_access, 5, "");
DEFINE_int32(max_protect_time, 150 * 1000000, "");
// 当一个站点连续抓取失败超过 多少次后 丢弃和该站点相关的所有 待抓取 URLs
DEFINE_int32(max_fail_cnt, 1000, "");
// 抓取 qps
DEFINE_int32(max_default_qps, 2, "");
// 爬虫的名字 User Agent
DEFINE_string(crawler_agent_name, "Mozilla/5.0 (Windows NT 5.1; rv:12.0) Gecko/20100101 Firefox/12.0",  "");
// 允许重定向 follow 的最大次数
DEFINE_int64(max_follow_count, 32, "");
// 是否显示抓取的详细信息, 一般在 debug 时使用: 1 开启, 0 关闭. 默认关闭
DEFINE_int64(debug_mode, 0, "");
// 对一个站点抓取多少次后, 实时计算 一次 QPS
DEFINE_int32(calculate_qps_every_n, 16, "");

// 需要同时支持 https 和 http 协议的 url 集合文件名字
DEFINE_string(url_file_support_https, "crawler/fetcher/url_support_https.txt", "");
// 需要同时支持 https 和 http 协议的 url 集合文件名字
DEFINE_string(url_file_need_proxy, "crawler/fetcher/url_need_proxy.txt", "");

namespace crawler {
// HTTP Header 的 Accept-Language 字段
const char *kAcceptLanguage = "Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3";
// kMaxHTMLSize is defined in crawler/proto/crawled_resource.proto
const uint64 kEachpage_body_bufferLen = kMaxHTMLSize;
const uint64 kEachpage_head_bufferLen = 8U << 10;

// 对于图片等信息，如果 |switch_crawler_whole_image| 为 true, 抓取全 image;
// 如果为 false, 由于只需要获取图片的高度和长度等信息 最多抓取前 1KB 信息
const uint64 kEachimg_body_bufferLen_part = (1U << 10);
const uint64 kEachimg_body_bufferLen_whole = kMaxHTMLSize;

const int kMaxEvent = 1024;

static size_t CallBackReceiveBodyData(char*, size_t, size_t, void*);
static size_t CallBackReceiveImageBodyData(char*, size_t, size_t, void*);
static size_t CallBackCurlReceiveHeadData(char*, size_t, size_t, void*);
static size_t CallBackCurlSocket(CURL*, curl_socket_t, int, void*, void*);
static int CallBackTimer(CURLM*, int64, void*);

MultiFetcher::MultiFetcher(TaskStatistics *stat, ResourceSaver *saver, const std::string &dns_servers,
                           int request_window_size,
                           const std::string &hostload_conf_file, int max_concurrent_access_fetcher) {
  CHECK(stat && saver);
  stat_ = stat;
  saver_ = saver;
  if (!dns_servers.empty()) {
    base::Tokenize(dns_servers, ",", &dns_servers_);
  }
  request_window_size_ = request_window_size;
  hostload_conf_file_ = hostload_conf_file;
  max_concurrent_access_fetcher_ = max_concurrent_access_fetcher;
}

bool MultiFetcher::Init() {
  curl_global_init(CURL_GLOBAL_ALL);
  curl_multi_handler_ = curl_multi_init();
  LOG_IF(FATAL, request_window_size_ <= 0 || request_window_size_ > 2001)
      << "request window size: " << request_window_size_<< ", not valid, should be [1, 2000].";

  LOG(INFO) << "libcurl version: " << curl_version();
  LOG(INFO) << "dns servers#: " << dns_servers_.size();
  if (FLAGS_switch_crawl_whole_image) {
    LOG(INFO) << "Will Store Whole Image";
  }

  page_head_buffer_ = new char[request_window_size_* kEachpage_head_bufferLen];
  LOG_IF(FATAL, page_head_buffer_ == NULL) << "Allocated mem for page_head_buffer fail, TOO big window_size?";
  page_body_buffer_ = new char[request_window_size_* kEachpage_body_bufferLen];
  LOG_IF(FATAL, page_body_buffer_ == NULL) << "Allocated mem for page_body_buffer fail, TOO big window_size?";

  page_head_ = new ByteBuffer[request_window_size_];
  LOG_IF(FATAL, page_head_ == NULL) << "new ByteBuffer[request_window_size_] fail";
  page_body_ = new ByteBuffer[request_window_size_];
  LOG_IF(FATAL, page_body_ == NULL) << "new ByteBuffer[request_window_size_] fail";

  buffer_free_flag_ = new bool[request_window_size_];
  LOG_IF(FATAL, buffer_free_flag_ == NULL) << "Allocated mem for buffer_free_flag fail.";
  for (int i = 0; i < request_window_size_; ++i) {
    buffer_free_flag_[i] = true;
  }
  overide_buffer_id_ = 0;

  host_access_stats_ = new std::map<std::string, HostLoadInfo>();
  CHECK_NOTNULL(host_access_stats_);
  host_access_paras_ = new std::map<std::string, std::vector<HostLoadControl>> ();
  CHECK_NOTNULL(host_access_paras_);
  LOG_IF(FATAL, !LoadHostControlRules(hostload_conf_file_, host_access_paras_,
                                      max_concurrent_access_fetcher_))
      << "Fail in LoadSiteControlRules()," << "rules files: " << hostload_conf_file_;

  CHECK_NOTNULL(saver_);

  epollfd_ = epoll_create(kMaxEvent);
  PLOG_IF(FATAL, epollfd_ == -1) << "Failed to create epollfd using epoll_create()";
  // we can optionally limit the total amount of connections this multi handle uses
  // 配合限速 控制最多连接个数 从而 整个爬虫占用带宽最大 200 × 2KB
  // curl_multi_setopt(curl_multi_handler_, CURLMOPT_MAXCONNECTS, 200);

  LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_,
                                              CURLMOPT_SOCKETFUNCTION, CallBackCurlSocket));
  LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETDATA, &epollfd_));

  LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERFUNCTION, &CallBackTimer));
  LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERDATA, &timeout_));
  // 显示忽略掉 SIGPIP 信号，因为在 DNS 异步解析时可能会产生这个神秘的信号(即使设置了 CURLOPT_NOSIGNAL)
  signal(SIGPIPE, SIG_IGN);

  url_set_support_https_ = new std::set<std::string> ();
  CHECK_NOTNULL(url_set_support_https_);
  InitUrlSetSupportHttps(FLAGS_url_file_support_https);

  url_set_need_proxy_ = new std::set<std::string> ();
  CHECK_NOTNULL(url_set_need_proxy_);
  InitUrlSetNeedProxy(FLAGS_url_file_need_proxy);

  return true;
}

void MultiFetcher::ResourceRelease() {
  curl_multi_cleanup(curl_multi_handler_);
  curl_global_cleanup();

  delete []page_body_buffer_;
  delete []page_head_buffer_;
  delete []page_head_;
  delete []page_body_;
  delete []buffer_free_flag_;
  delete host_access_stats_;
  delete host_access_paras_;
  delete url_set_support_https_;
  delete url_set_need_proxy_;

  close(epollfd_);
}

bool MultiFetcher::LoadHostControlRules(const std::string &file,
                                        std::map<std::string, std::vector<HostLoadControl>> *site_paras,
                                        int max_concurrent_access_fetcher) {
  CHECK_NOTNULL(site_paras);
  std::ifstream myfin(file);
  if (!myfin.good()) {
    LOG(ERROR) << "Fail in open file: " << file;
    return false;
  }
  std::string line;
  std::vector<std::string> tokens;
  site_paras->clear();
  int concurrent_size;
  int qps;
  int start_hour, start_min, end_hour, end_min;
  std::vector<HostLoadControl> rules;
  while (getline(myfin, line)) {
    base::TrimTrailingWhitespaces(&line);
    if (line.empty() || line[0] == '#') continue;
    tokens.clear();
    int num = base::Tokenize(line, " ", &tokens);
    // Format like: host
    if (num != 4) {
      LOG(ERROR) << "Record format error, field# should be 4, line: " << line;
      return false;
    }
    const std::string &ip = tokens[0];
    if (!base::StringToInt(tokens[1], &concurrent_size)) {  // concurrent_size
      LOG(ERROR) << "StringToInt() fail, concurrent_size: " << tokens[1];
      return false;
    }
    if (!base::StringToInt(tokens[2], &qps)) {  // max_qps
      LOG(ERROR) << "StringToInt() fail, qps: " << tokens[2];
      return false;
    }
    const std::string &time = tokens[3];
    if (!(time.size() == 11 && time[2] == ':' && time[5] == '-' && time[8] == ':')) {  // HH:MM-HH:MM
      LOG(ERROR) << "Time format error, must be: HH:MM-HH:MM, but is: " << time;
      return false;
    }
    std::string str_sh = time.substr(0, 2);
    std::string str_sm = time.substr(3, 2);
    std::string str_eh = time.substr(6, 2);
    std::string str_em = time.substr(9, 2);
    if (!(base::StringToInt(str_sh, &start_hour) &&
          base::StringToInt(str_sm, &start_min) &&
          base::StringToInt(str_eh, &end_hour) &&
          base::StringToInt(str_em, &end_min))) {
      LOG(ERROR) << "StringToInt() fail, time field: " << time;
      return false;
    }
    // 在配置文件配置全局 QPS, 对于每个 fetcher 能够达到的 qps = 全局 QPS / max_concurrent_access_fetcher;
    // XXX(pengdan): 不是严格小于 全局配置的 QPS, 但是 可以保证实际对 某一个 站点的 qps
    // 不会超过: 全局 QPS + max_concurrent_access_fetcher
    qps = qps / max_concurrent_access_fetcher + 1;
    concurrent_size= concurrent_size / max_concurrent_access_fetcher + 1;
    HostLoadControl sac(start_hour, start_min, end_hour, end_min, concurrent_size, qps);
    auto it = site_paras->find(ip);
    if (it == site_paras->end()) {
      std::vector<HostLoadControl> value;
      value.push_back(sac);
      site_paras->insert(std::make_pair(ip, value));
    } else {
      it->second.push_back(sac);
    }
  }
  CHECK(myfin.eof());
  return true;
}

void MultiFetcher::InitUrlSetSupportHttps(const std::string &filename) {
  CHECK(!filename.empty());
  std::ifstream myfin(filename);
  CHECK(myfin.good());

  url_set_support_https_->clear();

  std::string line;
  while (getline(myfin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty() || line[0] == '#') continue;
    // 归一化处理 ?
      url_set_support_https_->insert(line);
  }
  CHECK(myfin.eof());
  return;
}

void MultiFetcher::InitUrlSetNeedProxy(const std::string &filename) {
  CHECK(!filename.empty());
  std::ifstream myfin(filename);
  CHECK(myfin.good());

  url_set_need_proxy_->clear();

  std::string line;
  while (getline(myfin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty() || line[0] == '#') continue;
    // 归一化处理 ?
      url_set_need_proxy_->insert(line);
  }
  CHECK(myfin.eof());
  return;
}

void MultiFetcher::InitEasyInterface(const UrlHostIP &url_info) {
  int i = 0;
  while (i < request_window_size_ && !buffer_free_flag_[i]) ++i;
  if (i == request_window_size_) {
    LOG(WARNING) << "No buffer is avaiable for current session: " << url_info.url
                 << ", we will cleanup and reinit multi_handler";  // file handler leak ??
    curl_multi_cleanup(curl_multi_handler_);
    curl_multi_handler_ = curl_multi_init();
    LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_,
                                                CURLMOPT_SOCKETFUNCTION, CallBackCurlSocket));
    LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_SOCKETDATA, &epollfd_));

    LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERFUNCTION, &CallBackTimer));
    LOG_IF(FATAL, CURLM_OK != curl_multi_setopt(curl_multi_handler_, CURLMOPT_TIMERDATA, &timeout_));
    for (int j = 0; j < request_window_size_; ++j) {
      buffer_free_flag_[j] = true;
    }
    i = 0;
  }
  buffer_free_flag_[i] = false;
  CURL *eh = curl_easy_init();

  if (url_info.resource_type != kImage) {
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallBackReceiveBodyData);
  } else {
    // the image body may truncated to kEachimg_body_bufferLen(2KB),
    // because we just need the higth and width of image
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallBackReceiveImageBodyData);
  }
  page_body_[i].start = page_body_buffer_ + i * kEachpage_body_bufferLen;
  page_body_[i].len = 0;
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, page_body_ + i);

  curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &CallBackCurlReceiveHeadData);
  page_head_[i].start = page_head_buffer_ + i * kEachpage_head_bufferLen;
  page_head_[i].len = 0;
  curl_easy_setopt(eh, CURLOPT_HEADERDATA, page_head_ + i);

  curl_easy_setopt(eh, CURLOPT_USERAGENT, FLAGS_crawler_agent_name.c_str());
  // body include head or not?
  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  if (url_info.url.empty()) {
    buffer_free_flag_[i] = true;
    curl_easy_cleanup(eh);
    LOG(ERROR) << "Url in url_info is empty, ignore this url";
    return;
  } else {
    curl_easy_setopt(eh, CURLOPT_URL, url_info.url.c_str());
  }

  PrivateData *private_data = new PrivateData(i, url_info);
  LOG_IF(FATAL, NULL == private_data) << "new fail for private_data";
  curl_easy_setopt(eh, CURLOPT_PRIVATE, private_data);

  // 显示 libcurl 抓取的细节信息，一般在 debug 时使用
  curl_easy_setopt(eh, CURLOPT_VERBOSE, FLAGS_debug_mode);
  // libcurl 在名字解析超时时不会异常 crash
  curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L);
  // get the modification date of the remote document in this operation
  curl_easy_setopt(eh, CURLOPT_FILETIME, 1L);
  // 设置本次会话的超时（最大持续时间）
  curl_easy_setopt(eh, CURLOPT_TIMEOUT, FLAGS_max_http_session_time);
  // 支持 gzip 格式网页的自动解码
  curl_easy_setopt(eh, CURLOPT_ACCEPT_ENCODING, "");

  // 支持重定向 (http response code :3XX)
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
  // 设置重定向最大的 follow 次数，当超过该次数时，会返回： CURLE_TOO_MANY_REDIRECTS
  // Setting to 0 will make libcurl refuse any redirect, to -1 for an infinite number of redirects(default)
  curl_easy_setopt(eh, CURLOPT_MAXREDIRS, FLAGS_max_follow_count);
  // when seting to 1, libcurl will set the Referer: field in requests where it follows a Location: redirect
  curl_easy_setopt(eh, CURLOPT_AUTOREFERER, 1L);
  // Set the dns servers used in this request for name resolve: 该选项在 7.24.0 中支持
  if (!dns_servers_.empty()) {
    uint64 hashcode = base::CityHash64(url_info.site.c_str(), url_info.site.size());
    curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, dns_servers_[hashcode % dns_servers_.size()].c_str());
  }
  // 对于集合中的 urls, 同时支持 http 和 https 协议; 不在集合中的 urls, 仅 支持 http 协议
  auto it = url_set_support_https_->find(url_info.url);
  if (it == url_set_support_https_->end()) {
    curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP);
  } else {
    curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
  }
  // XXX(pengdan): 需要在本地开启 socks5 服务代理
  // 命令行: ssh -D 7070 onebox@106.187.51.232
  // password: 锄禾日当午
  // 对于需要 设置代理访问的 url; 设置 socks5 代理
  // it = url_set_need_proxy_->find(url_info.url);
  // if (it != url_set_need_proxy_->end()) {
  //  curl_easy_setopt(eh, CURLOPT_PROXY, "socks5h://localhost:7070");
  //  LOG(ERROR) << "Using proxy socks5 for url: " << url_info.url;
  // }
  // Support cookies
  curl_easy_setopt(eh, CURLOPT_COOKIEFILE, "");
  //  设置 Refer
  //  来至 更新模块的 URL 没有 refer 信息, 该字段存放 Last-modified-time
  if (url_info.from != 'M' && !url_info.refer_or_modified_time.empty()) {
    curl_easy_setopt(eh, CURLOPT_REFERER, url_info.refer_or_modified_time.c_str());
  }
  if (url_info.from == 'M' && !url_info.refer_or_modified_time.empty()) {
    std::string header_string = "If-Modified-Since: " + url_info.refer_or_modified_time;
    private_data->slist = curl_slist_append(private_data->slist, header_string.c_str());
  }
  // 设置 Header Accept-Language
  private_data->slist = curl_slist_append(private_data->slist, kAcceptLanguage);
  if (private_data->slist != NULL) {
    curl_easy_setopt(eh, CURLOPT_HTTPHEADER, private_data->slist);
  }

  curl_multi_add_handle(curl_multi_handler_, eh);

  // 实时压力计算
  RealtimeHostLoadAdjust(url_info.ip);
}

DEFINE_double_counter(crawler, download_byte, 0.0, "the total actual bytes downloaded");

bool MultiFetcher::ExtractInfoFromCurl(CURL *curl_handle, CrawledResource *info) {
  CHECK(curl_handle != NULL && info != NULL);
  PrivateData *private_data;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &private_data)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_PRIVATE fail";
    return false;
  }
  CHECK_NOTNULL(private_data);

  double score = private_data->url_info.importance;
  info->set_source_url(private_data->url_info.url);
  info->set_resource_type(private_data->url_info.resource_type);
  // get head  & body content
  char *head_start = page_head_[private_data->buffer_id].start;
  int32 head_len = page_head_[private_data->buffer_id].len;
  char *body_start = page_body_[private_data->buffer_id].start;
  int32 body_len = page_body_[private_data->buffer_id].len;

  info->set_http_header_raw(head_start, head_len);
  info->set_content_raw(body_start, body_len);

  // Only try to convert the page of HTML to utf-8
  if (private_data->url_info.resource_type == kHTML) {
    std::string content_utf8;
    const char *codepage = base::HTMLToUTF8(info->content_raw(), info->http_header_raw(), &content_utf8);
    info->set_content_utf8(content_utf8);
    if (codepage) {
      info->set_original_encoding(codepage);
    }
  }
  if (private_data->slist != NULL) {
    curl_slist_free_all(private_data->slist);
  }

  char *effective_url;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &effective_url)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_EFFECTIVE_URL fail";
    return false;
  }
  LOG_IF(FATAL, effective_url == NULL) << "effective url is NULL pointer";
  info->set_effective_url(effective_url);

  CrawlerInfo * crawl_info = info->mutable_crawler_info();
  std::string from = base::StringPrintf("%c", private_data->url_info.from);
  crawl_info->set_from(from.c_str());
  crawl_info->set_link_score(score);

  // XXX(pengdan): private data 已经 delete, 后面的代码不能在使用 private data
  delete private_data;

  int64 response_code;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_RESPONSE_CODE fail";
    return false;
  }
  crawl_info->set_response_code(response_code);

  int64 file_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_FILETIME, &file_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get URLINFO_FILETIME fail";
    return false;
  }
  crawl_info->set_file_time(file_time);
  crawl_info->set_crawler_timestamp(base::GetTimestamp());

  double crawl_duration;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME, &crawl_duration)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_TOTAL_TIME fail";
    return false;
  }
  crawl_info->set_crawler_duration(crawl_duration);

  double namelookup_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_NAMELOOKUP_TIME, &namelookup_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_NAMELOOKUP_TIME fail";
    return false;
  }
  crawl_info->set_namelookup_time(namelookup_time);

  double connect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_CONNECT_TIME, &connect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONNECT_TIME fail";
    return false;
  }
  crawl_info->set_connect_time(connect_time);

  double appconnect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_APPCONNECT_TIME, &appconnect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_APPCONNECT_TIME fail";
    return false;
  }
  crawl_info->set_appconnect_time(appconnect_time);

  double pretransfer_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_PRETRANSFER_TIME, &pretransfer_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CPRETRANSFER_TIME fail";
    return false;
  }
  crawl_info->set_pretransfer_time(pretransfer_time);

  double starttransfer_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_STARTTRANSFER_TIME, &starttransfer_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_STARTTRANSFER_TIME fail";
    return false;
  }
  crawl_info->set_starttransfer_time(starttransfer_time);

  double redirect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_TIME, &redirect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_TIME fail";
    return false;
  }
  crawl_info->set_redirect_time(redirect_time);

  int64 redirect_count;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_COUNT, &redirect_count)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_COUNT fail";
    return false;
  }
  crawl_info->set_redirect_count(redirect_count);

  double donwload_speed;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_SPEED_DOWNLOAD, &donwload_speed)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_SPEED_DOWNLOAD fail";
    return false;
  }
  crawl_info->set_donwload_speed(donwload_speed);

  int64 header_size;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_HEADER_SIZE, &header_size)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_HEADER_SIZE fail";
    return false;
  }
  crawl_info->set_header_size(header_size);
  COUNTERS_crawler__download_byte.Increase(header_size);

  double content_length_donwload;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                    &content_length_donwload)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONTENT_LENGTH_DOWNLOAD fail";
    return false;
  }
  crawl_info->set_content_length_donwload(content_length_donwload);
  if (content_length_donwload > 0) {
    COUNTERS_crawler__download_byte.Increase(content_length_donwload);
  }

  char *content_type;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &content_type)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONTENT_TYPE fail";
    return false;
  }
  if (content_type != NULL) {
    crawl_info->set_content_type(content_type);
  }
  // http body is truncated or not
  if (!info->has_is_truncated()) {
    info->set_is_truncated(false);
  }
  // update fail counter, set to 0
  crawl_info->set_update_fail_cnt(0);
  return true;
}

bool MultiFetcher::SimpleFetcher(const std::string &input_url, std::string *http_header,
                                 std::string *http_body, bool debug) {
  std::string url(input_url);
  base::TrimWhitespaces(&url);
  if (url.empty() || (http_header == NULL && http_body == NULL)) {
    LOG(ERROR) << "Fail: url.empty() || (http_header == NULL && http_body == NULL)";
    return false;
  }
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallBackReceiveBodyData);
  ByteBuffer body_buffer;
  body_buffer.len = 0;
  body_buffer.start = new char[kEachpage_body_bufferLen];
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, &body_buffer);

  curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &CallBackCurlReceiveHeadData);
  ByteBuffer head_buffer;
  head_buffer.len = 0;
  head_buffer.start = new char[kEachpage_head_bufferLen];
  curl_easy_setopt(eh, CURLOPT_HEADERDATA, &head_buffer);

  curl_easy_setopt(eh, CURLOPT_URL, url.c_str());
  curl_easy_setopt(eh, CURLOPT_USERAGENT, FLAGS_crawler_agent_name.c_str());
  // body include head or not?
  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  if (debug == true) {
    curl_easy_setopt(eh, CURLOPT_VERBOSE, 1L);
  } else {
    curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
  }

  // libcurl 在名字解析超时时不会异常 crash
  curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(eh, CURLOPT_TIMEOUT, 20);
  // 支持 gzip 格式网页的自动解码
  curl_easy_setopt(eh, CURLOPT_ACCEPT_ENCODING, "");
  // 支持重定向 (http response code :3XX)
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
  // 设置重定向最大的 follow 次数，当超过该次数时，会返回： CURLE_TOO_MANY_REDIRECTS
  // Setting to 0 will make libcurl refuse any redirect, to -1 for an infinite number of redirects(default)
  curl_easy_setopt(eh, CURLOPT_MAXREDIRS, FLAGS_max_follow_count);
  // when seting to 1, libcurl will set the Referer: field in requests where it follows a Location: redirect
  curl_easy_setopt(eh, CURLOPT_AUTOREFERER, 1L);
  // Only support http
  curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
  curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP);

  // curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, "192.168.11.58");
  // curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, "8.8.8.8");

  // Support cookies
  curl_easy_setopt(eh, CURLOPT_COOKIEFILE, "");
  // Set Header
  ::curl_slist *slist = NULL;
  slist =  curl_slist_append(slist, kAcceptLanguage);
  if (slist != NULL) {
    curl_easy_setopt(eh, CURLOPT_HTTPHEADER, slist);
  }

  CURLcode res = curl_easy_perform(eh);
  if (CURLE_OK != res) {
  LOG(ERROR) << "Failed, err code: " << res << ", msg: " << curl_easy_strerror(res) << ", url: " << url;
    delete [] head_buffer.start;
    delete [] body_buffer.start;
    if (slist != NULL) {
      curl_slist_free_all(slist);
    }
    curl_easy_cleanup(eh);
    return false;
  }
  int64 response_code;
  if (CURLE_OK != curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &response_code)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_RESPONSE_CODE fail";
    delete [] head_buffer.start;
    delete [] body_buffer.start;
    if (slist != NULL) {
      curl_slist_free_all(slist);
    }
    curl_easy_cleanup(eh);
    return false;
  }
  if (response_code != 200) {
    LOG(ERROR) << "Failed, response code: " << response_code << ", url: " << url;
    delete [] head_buffer.start;
    delete [] body_buffer.start;
    if (slist != NULL) {
      curl_slist_free_all(slist);
    }
    curl_easy_cleanup(eh);
    return false;
  }

  if (http_header) {
    std::string header(head_buffer.start, head_buffer.len);
    *http_header = header;
  }
  if (http_body) {
    std::string body(body_buffer.start, body_buffer.len);
    *http_body = body;
  }
  delete [] head_buffer.start;
  delete [] body_buffer.start;
  if (slist != NULL) {
    curl_slist_free_all(slist);
  }
  curl_easy_cleanup(eh);
  return true;
}

DEFINE_int64_counter(crawler, url_tried_num, 0, "urls# tried");
DEFINE_int64_counter(crawler, url_success_num, 0, "urls# success");
DEFINE_int64_counter(crawler, page_success_num, 0, "pages# success");
DEFINE_int64_counter(crawler, css_success_num, 0, "css# success");
DEFINE_int64_counter(crawler, image_success_num, 0, "image# success");
DEFINE_int64_counter(crawler, page_not_modified_num, 0, "page# not modified. return code 304");
DEFINE_int64_counter(crawler, failed_page_utf8_convert, 0, "page# failed in converting to utf8");
// Search Click
DEFINE_int64_counter(crawler, search_click, 0, "search clicked");
DEFINE_int64_counter(crawler, pv_rank, 0, "From PV log");
DEFINE_int64_counter(crawler, page_rank, 0, "Page Rank");
DEFINE_int64_counter(crawler, home_page, 0, "Url is the home page of a site");
DEFINE_int64_counter(crawler, directory_home_page, 0, "Url is the directory home page of a site");
DEFINE_int64_counter(crawler, high_quality_mutal_index_page, 0, "high_quality_mutal_index_page");
DEFINE_int64_counter(crawler, newlink, 0, "url from extracted by crawler");
DEFINE_int64_counter(crawler, navi, 0, "url navi boost data");
// VIP
DEFINE_int64_counter(crawler, vip_page, 0, "vip page#");
DEFINE_int64_counter(crawler, vip_css, 0, "vip css#");
DEFINE_int64_counter(crawler, vip_image, 0, "vip image#");

// 失败统计
DEFINE_int64_counter(crawler, err_timeout, 0, "");
DEFINE_int64_counter(crawler, err_4XX, 0, "including 403");
DEFINE_int64_counter(crawler, err_403, 0, "");
DEFINE_int64_counter(crawler, err_5XX, 0, "");
DEFINE_int64_counter(crawler, err_6, 0, "dns error");
DEFINE_int64_counter(crawler, err_7, 0, "connect to server error");
DEFINE_int64_counter(crawler, err_52, 0, "server didn’t reply anything");
DEFINE_int64_counter(crawler, err_56, 0, "failure in receiving network data");

void stat_err_info(int http_code, int curl_code) {
  if (curl_code == 0) {
    if (http_code >= 400 && http_code < 500) {
      COUNTERS_crawler__err_4XX.Increase(1);
      if (http_code == 403) {
        COUNTERS_crawler__err_403.Increase(1);
      }
    } else if (http_code >= 500) {
      COUNTERS_crawler__err_5XX.Increase(1);
    }
  } else {
    switch (curl_code) {
      case 28: {
        COUNTERS_crawler__err_timeout.Increase(1);
        break;
      }
      case 6: {
        COUNTERS_crawler__err_6.Increase(1);
        break;
      }
      case 7: {
        COUNTERS_crawler__err_7.Increase(1);
        break;
      }
      case 52: {
        COUNTERS_crawler__err_52.Increase(1);
        break;
      }
      case 56: {
        COUNTERS_crawler__err_56.Increase(1);
        break;
      }
    }
  }
}

void stat_cralwer_quality(const CrawledResource &info, const UrlHostIP &url_info) {
  GURL gurl(info.effective_url());
  if (url_info.from == 'S') {
    COUNTERS_crawler__search_click.Increase(1);
  }
  if (url_info.from == 'P') {
    COUNTERS_crawler__pv_rank.Increase(1);
  }
  if (url_info.from == 'N') {
    COUNTERS_crawler__newlink.Increase(1);
  }
  if (url_info.from == 'A') {
    COUNTERS_crawler__navi.Increase(1);
  }
  // Page Ranke
  // Site Home page
  const std::string &path = gurl.path();
  if (path == "/") {
    COUNTERS_crawler__home_page.Increase(1);
  }
  if (!base::MatchPattern(path, "*.*") || base::MatchPattern(path, "*/index.*") ||
      base::MatchPattern(path, "*/default.*")) {
    COUNTERS_crawler__directory_home_page.Increase(1);
  }
  // end 质量统计
}

void MultiFetcher::GetHostLoadParameter(const std::map<std::string, std::vector<HostLoadControl>> *site_paras,
                                        const std::string &ip, int *max_curr, int *qps, int *internal_id,
                                        int max_concurrent_access_fetcher) {
  CHECK_NOTNULL(site_paras);
  auto it = site_paras->find(ip);
  if (it != site_paras->end()) {  // 使用显示配置的压力参数
    const std::vector<HostLoadControl> &rules = it->second;
    // 获取当前的时间
    int64 t = time(NULL);
    struct tm  mytm = {0};
    struct tm * ptm = localtime_r(&t, &mytm);
    int hour = ptm->tm_hour;
    int min = ptm->tm_min;
    int min_from_00 = hour * 60 + min;  // 从 00 点开始的 分钟数
    for (int i = 0; i <(int)rules.size(); ++i) {
      int min_from_00_s = rules[i].start_hour * 60 + rules[i].start_min;
      int min_from_00_e = rules[i].end_hour * 60 + rules[i].end_min;
      if (min_from_00 >= min_from_00_s && min_from_00 <= min_from_00_e) {  // 闭区间
        if (max_curr != NULL) *max_curr = rules[i].max_concurrent_access;
        if (qps != NULL) *qps = rules[i].max_qps;
        if (internal_id != NULL) *internal_id = i;
        return;
      }
    }
  }
  // 没有显示配置该时间段 site 的压力, 使用 全局的默认压力控制
  if (max_curr != NULL) *max_curr = FLAGS_max_concurrent_site_access / max_concurrent_access_fetcher + 1;
  if (qps != NULL) *qps = FLAGS_max_default_qps / max_concurrent_access_fetcher + 1;
  if (internal_id != NULL) *internal_id = -1;
  return;
}

void MultiFetcher::RealtimeHostLoadAdjust(const std::string &ip) {
  // 清空状态
  HostLoadInfo &host_stats = (*host_access_stats_)[ip];
  host_stats.total_cnt++;  // 保护期内总访问次数加 1
  if (host_stats.total_cnt == 1) {
    host_stats.begin_timepoint = base::GetTimestamp();
  }
  /*
  // 统计抓取状态
  if ((http_response_code >= 200 && http_response_code < 300) || http_response_code == 304 ||
      http_response_code == 404) {
    host_stats.fail_cnt = 0;
    host_stats.success_cnt++;
  } else {
    host_stats.fail_cnt++;
    host_stats.success_cnt = 0;
  }
  if (host_stats.fail_cnt >= FLAGS_max_fail_cnt) {
    host_stats.expire_timepoint = -1;  // 超过失败次数上限,不再保护,直接丢弃和该 host 相关的待抓取链接
    return;
  }
  */

  // 压力保护
  //
  // 大于 |FLAGS_calculate_qps_every_n| 个后, 事实计算 QPS, 如果超过 既定设置,
  // 则休息 (保证 QPS 不超过设定值); 如果 没有达到 既定设置, 则加大并发窗口,
  // 向设定配置靠近
  if (host_stats.total_cnt > FLAGS_calculate_qps_every_n) {
    int64 consume_time = base::GetTimestamp() - host_stats.begin_timepoint;
    int config_qps, rule_id;
    GetHostLoadParameter(host_access_paras_, ip, NULL, &config_qps, &rule_id,
                         max_concurrent_access_fetcher_);
    int real_qps = (int)(1.0 * host_stats.total_cnt / (consume_time / 1000000.0));  // 升级为 double 计算
    if (real_qps > config_qps) {  // 抓取过快, 需要减慢抓取
      host_stats.expire_timepoint = host_stats.begin_timepoint +
          (int64)(1.0 * host_stats.total_cnt / config_qps * 1000000.0);
      int64 time_to_sleep = host_stats.expire_timepoint - host_stats.begin_timepoint - consume_time;
      LOG(ERROR) << "ip: " << ip << ", access num: " << host_stats.total_cnt
                 << ", comsumed time: " << consume_time << ", real qps: " << real_qps
                 << ", config qps: " << config_qps << ", begin time: " << host_stats.begin_timepoint
                 << ", expire time: " << host_stats.expire_timepoint << ", will sleep: "
                 << time_to_sleep << ", all time in micro seconds";
    } else if (real_qps < config_qps) {  // 抓取没有达到 QPS 配额, 需要加大并发窗口
      LOG(ERROR) << "ip: " << ip << ", access num: " << host_stats.total_cnt
                 << ", comsumed time: " << consume_time << ", real qps: " << real_qps
                 << ", config qps: " << config_qps << ", no need to sleep";
      if (rule_id != -1) {  // -1: 表示没有对该站点配置特定的 压力规则
        // TODO(pengdan): 加大并发窗口数
        /*
        auto it =  host_access_paras_->find(host);
        if (it != host_access_paras_->end()) {
          std::vector<HostLoadControl> &rules = it->second;
          rules[rule_id].max_concurrent_access++;  //
        }
        */
      }
    }
    // 开始下一个保护周期
    host_stats.total_cnt = 0;
    // host_stats.begin_timepoint = host_stats.expire_timepoint;
  }
}

void MultiFetcher::HandleCurlMessage(CURLMsg *msg) {
  COUNTERS_crawler__url_tried_num.Increase(1);
  PrivateData *private_data;
  CURL *easy_handle = msg->easy_handle;
  LOG_IF(FATAL, CURLE_OK != curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &private_data))
      << "Get private data curl_easy_getinfo() fail.";
  CrawledResource info;
  UrlHostIP url_info(private_data->url_info);
  int64 begin_timepoint = private_data->begin_timepoint;
  buffer_free_flag_[private_data->buffer_id] = true;
  int http_response_code = 0;
  switch (msg->data.result) {
    case 23: {  // 截断 body Err code: 23, msg: Failed writing received data to disk/application
      info.set_is_truncated(true);  // No need break here
      LOG_IF(ERROR, url_info.resource_type != kImage)
          << "Err code: " << msg->data.result << ", msg: " << curl_easy_strerror(msg->data.result)
          << ", url: " << url_info.url;
    }
    case 0: {  // 会话正常结束
      if (!ExtractInfoFromCurl(easy_handle, &info)) {
        LOG(ERROR) << "ExtractInfoFromCurl() fail, current session will be ignored.";
        break;
      }
      http_response_code = info.crawler_info().response_code();
      if ((http_response_code >= 200 && http_response_code < 300) ||
          (http_response_code >=400 && http_response_code < 500 && http_response_code != 403)) {
        COUNTERS_crawler__url_success_num.Increase(1);
        if (url_info.resource_type == kHTML) {
          COUNTERS_crawler__page_success_num.Increase(1);
          // 使用严格的 过滤规则, 因为对于搜索引擎的结果页不用存储,只需要提取链接
          std::string reason;
          bool status = false;
          // if (FLAGS_switch_filter_rules == true) {
          // status = WillFilterAccordingRules(url_info.url, &reason, true);
          // }
          if (status == false) {
            // 解析网页得到 html_tree, 并计算 html_body 的 simhash, 用于网页 dedup
            // 对于 UTF8 转换失败的网页, 无法计算其 simhash, 设置该 field 为 0
            const std::string &page_utf8 = info.content_utf8();
            if (page_utf8.empty()) {
              info.set_simhash(0);
              COUNTERS_crawler__failed_page_utf8_convert.Increase(1);
              LOG(ERROR) << "failed_page_utf8 url:" << info.source_url();
            } else {
              info.set_simhash(0);
            }
            if (!saver_->StoreProtoBuffer(info, 'P')) {
              LOG(ERROR) << "Store WebPage fail, ignore url: " << url_info.url;
              break;
            }
            // 提取新的链接 css/image link
            if (!saver_->ExtractAndStoreNewLink(info, true, true)) {
              LOG(ERROR) << "saver->ExtractAndStoreNewLink(), ignore new links in: " << url_info.url;
              break;
            }
          } else {  // 如果是搜索引擎 url, 仅仅提取新的 link, 不需要提取 image/css
            if (!saver_->ExtractAndStoreNewLink(info, false, false)) {
              LOG(ERROR) << "saver->ExtractAndStoreNewLink(), ignore new links in: " << url_info.url;
              break;
            }
          }
          stat_->link_fetch_ok_num++;
          LOG(ERROR) << "done, url: " << url_info.url;
          // 抓取质量统计
          stat_cralwer_quality(info, url_info);
        } else if (url_info.resource_type == kCSS) {  // store css
          COUNTERS_crawler__css_success_num.Increase(1);
          if (!saver_->StoreProtoBuffer(info, 'C')) {
            LOG(ERROR) << "Store Css fail and ignore this url, src url: " << url_info.url;
            break;
          }
          stat_->css_fetch_num++;
        } else if (url_info.resource_type == kImage) {  // store image
          COUNTERS_crawler__image_success_num.Increase(1);
          if (!saver_->StoreProtoBuffer(info, 'I')) {
            LOG(ERROR) << "Store Image fail and ignore this url, src url: " << url_info.url;
            break;
          }
          stat_->image_fetch_num++;
        } else {
          LOG(ERROR) << "Unkown resource type:" << url_info.resource_type
                     <<  ", only support html, css and image.";
          break;
        }
      } else if (http_response_code == 304) {  // Page Not Modified!
        COUNTERS_crawler__page_not_modified_num.Increase(1);
      } else {  // http 请求没有正常处理
        LOG(ERROR) << "Http response code: " << http_response_code
                   << ", url: " << url_info.url << ", score: " << url_info.importance;
        stat_->link_failed_get_page_num++;
      }
      ReportCrawlStatus(saver_, msg->data.result, http_response_code, url_info.url, begin_timepoint,
                   info.crawler_info().redirect_count(), info.effective_url(), url_info.payload);
      break;
    }
    case 28:  {  // 会话超时
      stat_->link_fetch_timeout_num++;
    }
    case 6:  // Couldn't resolve host name
    case 7:  // Couldn't connect to server
    case 52:  // Server returned nothing (no headers, no data)
    case 56:  // Failure when receiving data from the peer
    default: {
      LOG(ERROR) << "Err code: " << msg->data.result << ", msg: " << curl_easy_strerror(msg->data.result)
                 << ", url: " << url_info.url << ", score: " << url_info.importance;
      stat_->link_failed_get_page_num++;
      ReportCrawlStatus(saver_, msg->data.result, -1, url_info.url, begin_timepoint, 0, "", url_info.payload);
    }
  }
  stat_err_info(http_response_code, msg->data.result);

  // 并发连接数 减 1
  HostLoadInfo &host_stats = (*host_access_stats_)[url_info.ip];
  host_stats.concurrent_access_cnt--;  // host 访问并发数减 1

  // RealtimeHostLoadAdjust(url_info.ip);

  curl_multi_remove_handle(curl_multi_handler_, easy_handle);
  curl_easy_cleanup(easy_handle);
}

bool MultiFetcher::MultiCrawlerAndAnalyze(std::vector<UrlHostIP> *urls) {
  int64 actual_window_size;
  int32 running;
  int32 msg_in_queue;
  int64 timestamp;
  struct epoll_event events[kMaxEvent];
  std::vector<UrlHostIP>::iterator it;
  CURLMsg *msg;
  actual_window_size = request_window_size_;
  int64 task_number = static_cast<int64>(urls->size());
  if (actual_window_size > task_number) {
    actual_window_size = task_number;
  }
  host_access_stats_->clear();
  int64 counter = 0;
  for (it = urls->begin(); counter < actual_window_size && it != urls->end(); ++it) {
    HostLoadInfo &host_stats = (*host_access_stats_ )[it->ip];
    int max_cur;
    GetHostLoadParameter(host_access_paras_, it->ip, &max_cur, NULL, NULL, max_concurrent_access_fetcher_);
    if (host_stats.concurrent_access_cnt > max_cur) continue;
    // 没有被调度 而且 对该站点的并非访问没有超过最大上限
    InitEasyInterface(*it);
    it->status = '1';  // 0: 没有调度  1: 已经调度过 or 被丢弃
    host_stats.concurrent_access_cnt++;
    ++counter;
  }
  curl_multi_socket_action(curl_multi_handler_, CURL_SOCKET_TIMEOUT, 0, &running);
  // XXX(pengdan): 可能存在情况: 链接相关站点因为过载保护而没有被调度, 但是 running
  // 数为 0 (没有活动的 http session) 而退出程序.
  while (running) {
    curl_multi_socket_all(curl_multi_handler_, &running);
    if (running) {
      curl_multi_socket_action(curl_multi_handler_, CURL_SOCKET_TIMEOUT, 0, &running);
      if (timeout_ == -1) timeout_ = 100;  // wait 100 millionSeconds
      int status = epoll_wait(epollfd_, events, kMaxEvent, timeout_);
      // 被更高级别系统调用中断，较常出现，继续 wait
      LOG_IF(FATAL, status == -1 && errno != EINTR) << "epoll_wait() fail, errno: " << errno;
    }
    while ((msg = curl_multi_info_read(curl_multi_handler_, &msg_in_queue))) {
      PLOG_IF(FATAL, msg->msg != CURLMSG_DONE) << "Transfer session fail, err msg: " << msg->msg;
      HandleCurlMessage(msg);
      // 新分配一个 http session 给 一个待抓取的链接
      while (counter < task_number) {
        if (it == urls->end()) it = urls->begin();
        HostLoadInfo &host_stats = (*host_access_stats_ )[it->ip];
        int max_cur;
        GetHostLoadParameter(host_access_paras_, it->ip, &max_cur, NULL, NULL,
                             max_concurrent_access_fetcher_);
        int access_times = host_stats.concurrent_access_cnt;
        // 已经被调度处理过 或者 该 url 的 host 超过了最大并发访问数
        if (it->status != '0' || access_times > max_cur) {
          if (it->status != '0') {  // 已经被调度过的 url, 从队列中删除 (唯一一处进行 erase 操作)
            it = urls->erase(it);
          } else {
          ++it;
          }
          if (it == urls->end()) {
            break;
          } else {
            continue;
          }
        }
        // 没有被调度 而且 对该 host 的并发访问没有超过上限
        int64 expire = host_stats.expire_timepoint;
        if (expire == -1) {  // 该站点可能已经封禁了本爬虫, 丢弃本次对该站点的所有未抓取链接
          it = urls->erase(it);
          ++counter;
          continue;
        }
        timestamp = base::GetTimestamp();
        if (timestamp < expire) {  // 该站点还在保护期内, 跳过并获取下一个站点 (其他站点) 链接
          ++it;
          continue;
        }
        InitEasyInterface(*it);
        it->status = '1';
        host_stats.concurrent_access_cnt++;
        ++it;
        ++counter;
        break;
      }  // end while (counter < task_number)
    }  // end while (curl_multi_info_read())
  }  // end while (running)
  return true;
}

size_t CallBackReceiveBodyData(char* data, size_t num,
                               size_t unit_size, void* user_data) {
  ByteBuffer* byte_buf = static_cast<ByteBuffer *>(user_data);
  if (!byte_buf) {
    LOG(WARNING) << "user_data in CallBackReceiveBodyData is NULL pointer";
    return 0;
  }
  int32 offset = byte_buf->len;
  char *buf = byte_buf->start;
  if (!buf) {
    LOG(WARNING) << "buf in CallBackReceiveBodyData is NULL pointer";
    return 0;
  }
  if (offset + num * unit_size > kEachpage_body_bufferLen) {
    LOG(WARNING) << "Page size: " << offset + num * unit_size << ", exceed max size: "
                 << kEachpage_body_bufferLen << ", truncated.";
    memcpy(buf + offset, data, kEachpage_body_bufferLen - offset);
    byte_buf->len = kEachpage_body_bufferLen;
    // 为了避免截断一个汉字或其他编码字符，回退到最后一个 tag  结尾符 ‘>’
    int i = byte_buf->len - 1;
    for (; i > 0 && buf[i] != '>'; --i);
    byte_buf->len = i+1;
    return (kEachpage_body_bufferLen - offset);
  } else {
    memcpy(buf + offset, data, num * unit_size);
    byte_buf->len += num * unit_size;
    return num * unit_size;
  }
}

size_t CallBackReceiveImageBodyData(char* data, size_t num,
                                    size_t unit_size, void* user_data) {
  ByteBuffer* byte_buf = static_cast<ByteBuffer*>(user_data);
  if (!byte_buf) {
    LOG(WARNING) << "user_data in CallBackReceiveImageBodyData is NULL pointer";
    return 0;
  }
  int32 offset = byte_buf->len;
  char *buf = byte_buf->start;
  if (!buf) {
    LOG(WARNING) << "buf in CallBackReceiveImageBodyData is NULL pointer";
    return 0;
  }
  uint64 image_body_max_length;
  if (FLAGS_switch_crawl_whole_image == true) {
    image_body_max_length = kEachimg_body_bufferLen_whole;
  } else {
    image_body_max_length = kEachimg_body_bufferLen_part;
  }
  if (offset + num * unit_size > image_body_max_length) {
    memcpy(buf + offset, data, image_body_max_length - offset);
    byte_buf->len = image_body_max_length;
    return (image_body_max_length - offset);
  } else {
    memcpy(buf + offset, data, num * unit_size);
    byte_buf->len += num * unit_size;
    return num * unit_size;
  }
}

size_t CallBackCurlReceiveHeadData(char* data, size_t num,
                                   size_t unit_size, void* user_data) {
  ByteBuffer* byte_buf = static_cast<ByteBuffer*>(user_data);
  if (!byte_buf) {
    LOG(WARNING) << "user_data in CallBackReceiveImageHeadData is NULL pointer";
    return 0;
  }
  int32 offset = byte_buf->len;
  char *buf = byte_buf->start;
  if (!buf) {
    LOG(WARNING) << "buf in CallBackReceiveImageHeadData is NULL pointer";
    return 0;
  }
  if (offset + num * unit_size > kEachpage_head_bufferLen) {
    LOG(WARNING) << "Header size: "<< offset + num * unit_size<< ", exceed max size: "
                 << kEachpage_head_bufferLen << ", truncated.";
    memcpy(buf + offset, data, kEachpage_head_bufferLen - offset);
    byte_buf->len = kEachpage_head_bufferLen;
    return (kEachpage_head_bufferLen - offset);
  } else {
    memcpy(buf + offset, data, num * unit_size);
    byte_buf->len += num * unit_size;
    return num * unit_size;
  }
}


size_t CallBackCurlSocket(CURL *easy, curl_socket_t s,
                          int action, void *userp, void *socketp) {
  int epollfd  = *(static_cast<int *>(userp));
  struct epoll_event ev;
  switch (action) {
    case CURL_POLL_IN: {
      ev.events = EPOLLIN;
      ev.data.fd = s;
      if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
        epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
      }
      break;
    }
    case CURL_POLL_OUT: {
      ev.events = EPOLLOUT;
      ev.data.fd = s;
      if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
        epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
      }
      break;
    }
    case CURL_POLL_REMOVE: {
      ev.events = EPOLLOUT;
      ev.data.fd = s;
      epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
      break;
    }
    default: {
      LOG(ERROR) << "Action is not CURL_POLL_IN, CURL_POLL_OUT or CURL_POLL_REMOVE.";
    }
  }
  return 0;
}

int CallBackTimer(CURLM *cm, int64 timeout_ms, void *userp) {
  int64 * timeout_ptr = static_cast<int64_t*>(userp);
  *timeout_ptr = timeout_ms;
  return 0;
}

void ReportDnsResolveStatus(ResourceSaver *saver, const std::string &url, const std::string &reason,
                            const std::string &payload) {
  CHECK_NOTNULL(saver);
  ReportStatus status;
  status.url = url;
  status.start_time = base::GetTimestamp();
  status.end_time = base::GetTimestamp();
  status.success = false;
  status.reason = reason;
  status.redir_times = 0;
  status.effective_url = "";
  status.pay_load = payload;
  LOG_IF(ERROR, !saver->StoreStatus(status)) << "Fail: saver->StoreStatus(), status serialize to string: "
                                             << SerializeReportStatus(status);
  return;
}

void ReportCrawlStatus(ResourceSaver *saver, int curl_code, int http_response_code, const std::string &url,
                       int64 start_timepoint, int32 redir_times, const std::string &effective_url,
                       const std::string &payload) {
  CHECK_NOTNULL(saver);
  ReportStatus status;
  status.url = url;
  status.start_time = start_timepoint;
  status.end_time = base::GetTimestamp();
  status.success = true;
  status.redir_times = redir_times;
  status.effective_url = effective_url;
  status.pay_load = payload;
  if (curl_code == 0) {
    if (http_response_code >= 500 || http_response_code == 403) {
      status.success = false;
    }
    status.reason = base::StringPrintf("%d", http_response_code);
  } else {
    status.success = false;
    status.reason = curl_easy_strerror((CURLcode)curl_code);
  }
  LOG_IF(ERROR, !saver->StoreStatus(status)) << "Fail: saver->StoreStatus(), status serialize to string: "
                                             << SerializeReportStatus(status);
  return;
}

}  // end namespace
