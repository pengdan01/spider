#include "crawler/crawl/page_crawler.h"

#include "base/encoding/url_encode.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/time/timestamp.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"

#include "crawler/crawl/crawl_util.h"

namespace crawler2 {
namespace crawl {

DEFINE_string(cookie_file_path, ".curl_cookie.txt", "存放会话过程中接收到 web server 的 cookies");
DEFINE_string(baidu_refer_url, "http://www.baidu.com/search/ressafe.html", "");

namespace {
const char *kAcceptLanguage = "Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3";
}  // namespace

void PageCrawler::SetProxyUserAgent(CURL *eh, const CrawlTask *task) {
  // 默认使用全局配置中的代理
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_USERAGENT, options_.user_agent.c_str()), CURLE_OK);

  // 设置代理
  if (!task->proxy.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROXY,
                              task->proxy.c_str()), CURLE_OK);
    // 对于使用代理的 url, 使用代理 UA user_agent_p
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_USERAGENT, options_.user_agent_p.c_str()), CURLE_OK);
  }

  // 如果本 task 指定了 UA, 则使用指定的 UA
  if (!task->user_agent.empty()) {
    // 对于使用代理的 url, 使用代理 UA user_agent_p
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_USERAGENT, task->user_agent.c_str()), CURLE_OK);
  }

  return;
}

void PageCrawler::SetEasyHandler(CURL *eh, CrawlRoutineData *routine) {
  VLOG(2) << "set easy hanler for url: " << routine->task->url;
  routine->status->begin_time = ::base::GetTimestamp();

  GURL gurl(routine->task->url);
  CHECK(gurl.is_valid()) << "Invalid URL: " << routine->task->url;

  // 创建抓取过程中需要的各种临时对象 (private data)
  CURLData& private_data = routine->private_data;
  // 设置请求的 http 头
  {
    // Accept-Language
    private_data.request_header = curl_slist_append(private_data.request_header, kAcceptLanguage);
    // Host
    std::string host_para = "Host: " + gurl.host();
    private_data.request_header = curl_slist_append(private_data.request_header, host_para.c_str());
  }

  // XXX(suhua): 如果 url 里没有写明端口, gurl.port() 不会返回 80 (for http),
  // 从而使得生成的 dns_cache 字符串在填到 libcurl 中时失效.
  // 解决方案: dns_cache 字符串 hard code 常量 80.
  // XXX(suhua): 这里 hard code 了 80 端口, 因此 dns_cache 只对 http 协议生效.
  // 还好, 我们暂时只抓 http 资源, 少量 https 资源不能用 dns_cache 基本无影响.
  // std::string dns_cache = gurl.host() + ":" + gurl.port() + ":" + url_to_fetch.ip;
  // std::string dns_cache = gurl.host() + ":80:" + url_to_fetch.ip;
  // XXX(pengdan): 使用 gurl.EffectiveIntPort() 获取端口, 如果 获取失败, 则硬编码为 80
  int port = gurl.EffectiveIntPort();
  if (port == url_parse::PORT_UNSPECIFIED) {
    port = 80;
  }

  if (!routine->task->ip.empty()) {
    std::string dns_cache = base::StringPrintf("%s:%d:%s",
                                               gurl.host().c_str(),
                                               port,
                                               routine->task->ip.c_str());
    private_data.host_ip = curl_slist_append(private_data.host_ip, dns_cache.c_str());
  }

  // 设置各种必要参数
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallbackBodyData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEDATA, routine), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &CallbackHeadData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERDATA, routine), CURLE_OK);

  // body include head or not? we say no
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADER, 0L), CURLE_OK);

  // XXX(pengdan): 对于在集合 |client_redirect_urls_| 的 url 需要进行特殊 hack 处理:
  // 实际抓取的 url 为 hack 后的 url
  {
    auto it = client_redirect_urls_.find(routine->task->url);
    if (it == client_redirect_urls_.end()) {
      CHECK_EQ(curl_easy_setopt(eh, CURLOPT_URL, routine->task->url.c_str()), CURLE_OK);
    } else {
      const std::string &hacked_url = it->second;
      CHECK_EQ(curl_easy_setopt(eh, CURLOPT_URL, hacked_url.c_str()), CURLE_OK);
      LOG(INFO) << "url: " << it->first << ", hacked to: " << it->second;
    }
  }
  // 对于 AJAX url, 使用转换后的 url
  {
    const std::string &url = routine->task->url;
    if (IsAjaxUrl(url)) {
      std::string converted_url;
      TransformAjaxUrl(url, &converted_url);
      CHECK_EQ(curl_easy_setopt(eh, CURLOPT_URL, converted_url.c_str()), CURLE_OK);
      LOG(INFO) << "url: " << url << ", is AJAX url and converted to: " << converted_url;
    }
  }

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PRIVATE, routine), CURLE_OK);

  // 显示 libcurl 抓取的细节信息，一般在 debug 时使用
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_VERBOSE,
                            (long)options_.enable_curl_debug), CURLE_OK);  // NOLINT
  // libcurl 在名字解析超时时不会异常 crash
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L), CURLE_OK);
  // get the modification date of the remote document in this operation
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_FILETIME, 1L), CURLE_OK);
  // 建立连接超时
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT,
                            (long)options_.connect_timeout_in_seconds), CURLE_OK);  // NOLINT
  // 传输超时
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_TIMEOUT,
                            (long)options_.transfer_timeout_in_seconds), CURLE_OK);  // NOLINT
  // 传输过慢, 过慢时间限制
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_LOW_SPEED_LIMIT,
                            (long)options_.low_speed_limit_in_bytes_per_second), CURLE_OK);  // NOLINT
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_LOW_SPEED_TIME,
                            (long)options_.low_speed_duration_in_seconds), CURLE_OK);  // NOLINT

  // 支持 gzip 格式网页的自动解码
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_ACCEPT_ENCODING, ""), CURLE_OK);
  // 支持重定向 (http response code :3XX)
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L), CURLE_OK);
  // 设置重定向最大的 follow 次数，当超过该次数时，会返回： CURLE_TOO_MANY_REDIRECTS
  // Setting to 0 will make libcurl refuse any redirect, to -1 for an infinite number of redirects(default)
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_MAXREDIRS, (long)options_.max_redir_times), CURLE_OK);  // NOLINT
  // when seting to 1, libcurl will set the Referer: field in requests where it follows a Location: redirect
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_AUTOREFERER, 1L), CURLE_OK);
  // Set the dns servers used in this request for name resolve: 该选项在 7.24.0 中支持
  if (!routine->task->dns.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, routine->task->dns.c_str()), CURLE_OK);
  }

  // 对于集合中需要特殊处理的 urls(https support or hacking url),
  // 同时支持 http 和 https 协议; 不在集合中的 urls, 仅 支持 http 协议
  // 不再做限制, 都支持 http 和 https
  {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);
  }

  // support cookies, 不然 蘑菇街 等首页抓取不能
  // CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEFILE, FLAGS_cookie_file_path.c_str()), CURLE_OK);
  // CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEJAR, FLAGS_cookie_file_path.c_str()), CURLE_OK);

  // add by meng.yuym 不让curl把cookie落地成文件，只放在内存里使用即可，参考的
  // https://stackoverflow.com/questions/1486099/any-way-to-keep-curls-cookies-in-memory-and-not-on-disk
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEFILE, ""), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEJAR, "/dev/null"), CURLE_OK);

  // XXX(pengdan): 由外部的一个脚本定时清 cookie, 不用在 curl 程序内部清理
  //  设置 refer
  if (!routine->task->referer.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REFERER, routine->task->referer.c_str()), CURLE_OK);
  }

  // 设置 header Accept-Language
  if (private_data.request_header != NULL) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HTTPHEADER,
                              routine->private_data.request_header), CURLE_OK);
  }

  SetProxyUserAgent(eh, routine->task);

  if (!routine->task->ip.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_RESOLVE, private_data.host_ip), CURLE_OK);
  }

  if (routine->task->always_create_new_link) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_FRESH_CONNECT, 1L), CURLE_OK);
  }

  // 不进行 SSL 证书验证
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L), CURLE_OK);

  // 是否只包含头部, 即: Head 请求
  // 0: get/post method; 1: head method
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_NOBODY, options_.nobody), CURLE_OK);

  // 设置 cookie for outgoing request
  if (!routine->task->cookie.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIE, routine->task->cookie.c_str()), CURLE_OK);
  }

  // 设置 post data for POST request
  if (!routine->task->post_body.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_POST, 1L), CURLE_OK);
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_POSTFIELDS, routine->task->post_body.c_str()), CURLE_OK);
  }
}

bool PageCrawler::ExtractInfoFromCurl(CURLMsg *msg, CrawlRoutineData *routine) {
  CURL *curl_handle = msg->easy_handle;
  CURLData* private_data = &routine->private_data;

  // 设置 url fected 的简单成员
  // ===============================================
  if (msg->data.result != 0 && msg->data.result != 23) {
    // 若抓取失败, 则仅设置少量结果, 即返回
    routine->status->success = false;
    routine->status->ret_code = msg->data.result;
    routine->status->error_desc = curl_easy_strerror(msg->data.result);
    return true;
  }

  int64 http_response_code;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_response_code)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_RESPONSE_CODE fail";
    return false;
  }

  // XXX(pengdan): 当代理服务达到最大链接数限制时, 会拒接本地的链接请求, 但是 http response 为 0
  // 且 curl code 为 0. 这种情况也判断为失败
  if (http_response_code == 0) {
    routine->status->success = false;
    routine->status->ret_code = msg->data.result;
    routine->status->error_desc = curl_easy_strerror(msg->data.result);
    return true;
  }

  // 抓取成功, 才会继续提取信息

  // XXX(suhua):
  // 之前, 需要有 http response code 且不是 500 和 403 才算抓取成功
  // 现在改为只要有 response code, 就认为抓取成功.
  // if (http_response_code >= 500 || http_response_code == 403) {
  //   url_fetched->success = false;
  // } else {
  //   url_fetched->success = true;
  // }
  routine->status->success = true;
  routine->status->ret_code = http_response_code;

  int64 redirect_count;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_COUNT,
                                    &redirect_count)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_COUNT fail";
    return false;
  }
  routine->status->redir_times = redirect_count;

  char *raw_effective_url;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL,
                                    &raw_effective_url)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_EFFECTIVE_URL fail";
    return false;
  }
  LOG_IF(FATAL, raw_effective_url == NULL) << "effective url is NULL pointer";

  // XXX(suhua): raw_effective_url 中可能包含 中文、空格、TAB 等, 需要处理一下
  const char *code_page = base::FindCodepageInHTML(private_data->body);
  if (code_page == NULL) {
    code_page = base::FindCodepageInHTTPHeader(private_data->head);
  }
  if (code_page != NULL) {
    if (strcmp(code_page, "GBK") == 0 || strcmp(code_page, "GB2312") == 0) {
      code_page = "GB18030";
    }
  }
  // TODO(suhua): 这里将 HTML 和 response header 里找到的编码, 当作 effective
  // url 的编码, 不一定正确, 需要验证
  if (!web::RawToClickUrl(raw_effective_url, code_page, &routine->status->effective_url)) {
    // XXX(suhua): 如果转换失败, 则简单处理一下, 避免后续输出错误的文本文件
    std::string url = raw_effective_url;
    url = base::StringReplace(url, "\t", base::EncodeUrlComponent("\t"), true);
    url = base::StringReplace(url, "\n", base::EncodeUrlComponent("\n"), true);
    url = base::StringReplace(url, "\r", base::EncodeUrlComponent("\r"), true);
    url = base::StringReplace(url, " ", base::EncodeUrlComponent(" "), true);
    routine->status->effective_url = url;
  }

  // XXX(pengdan): 对 hack 过的 url, 其 effective url 需要设置成和 source url 一样
  {
    auto it = client_redirect_urls_.find(routine->task->url);
    if (it != client_redirect_urls_.end()) {
      routine->status->effective_url = routine->task->url;
    }
  }
  // 对于 AJAX url, 其 effective url 需要设置成和 source url 一样
  {
    if (IsAjaxUrl(routine->task->url)) {
      routine->status->effective_url = routine->task->url;
    }
  }

  routine->status->file_bytes = private_data->body.size();

  double download_bytes;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &download_bytes)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_COUNT fail";
    return false;
  }
  routine->status->download_bytes = (int64)download_bytes;

  if (msg->data.result == 23) {
    routine->status->truncated = true;
  } else {
    routine->status->truncated = false;
  }

  // TODO(suhua): 解析 http response header, 识别下载下来的资源的真实 resource type, 并修改 proto buffer

  // 设置 crawled resource
  // ====================================================
  // 3. 若 抓取成功, 则设置 url_fetched 的成员 proto Resource
  Resource *res = routine->res;

  Brief *brief = res->mutable_brief();
  Content *content = res->mutable_content();

  brief->set_source_url(routine->task->url);
  brief->set_resource_type((ResourceType)routine->task->resource_type);
  if (!routine->task->referer.empty()) {
    brief->set_referer(routine->task->referer);
  }

  // get head  & body content
  brief->set_response_header(private_data->head);

  content->set_raw_content(private_data->body);

  // XXX(suhua): 不要在这里尝试转码, fetcher 在的主线程只负责抓取,
  // 不能做任何消耗 CPU 的事情.  这些事情留给其他专门做分析的线程去做.
  //
  brief->set_effective_url(routine->status->effective_url);


  // 根据 click effective url 生成 url
  // XXX(suhua): brief->effective_url() 在上面处理过, 已经是 click url 了
  const std::string &effective_click_url = brief->effective_url();
  std::string url;
  if (url_norm_->NormalizeClickUrl(effective_click_url, &url)) {
    brief->set_url(url);
  } else {
    LOG(WARNING) << "failed to normalize effective click url: "
                 << effective_click_url;
    brief->set_url(effective_click_url);
  }

  // http body is truncated or not
  brief->set_is_truncated(routine->status->truncated);


  // 设置 crawler info
  // ====================================================
  CrawlInfo *crawl_info = brief->mutable_crawl_info();

  // TODO(suhua): score 需要保证是归一化的
  double score = routine->task->importance;
  crawl_info->set_score(score);

  // TODO(suhua): 可以不设置 from?
  // std::string from = base::StringPrintf("%c", private_data->url_info.from);
  // crawl_info->set_from(from.c_str());

  int64 response_code;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE,
                                    &response_code)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_RESPONSE_CODE fail";
    return false;
  }
  crawl_info->set_response_code(response_code);

  int64 file_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_FILETIME,
                                    &file_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get URLINFO_FILETIME fail";
    return false;
  }
  crawl_info->set_file_timestamp(file_time);
  crawl_info->set_crawl_timestamp(base::GetTimestamp());

  double crawl_duration;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_TOTAL_TIME,
                                    &crawl_duration)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_TOTAL_TIME fail";
    return false;
  }
  crawl_info->set_crawl_time(crawl_duration);

  double connect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_CONNECT_TIME,
                                    &connect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONNECT_TIME fail";
    return false;
  }
  crawl_info->set_connect_time(connect_time);

  double appconnect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_APPCONNECT_TIME,
                                    &appconnect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_APPCONNECT_TIME fail";
    return false;
  }
  crawl_info->set_appconnect_time(appconnect_time);

  double pretransfer_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_PRETRANSFER_TIME,
                                    &pretransfer_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CPRETRANSFER_TIME fail";
    return false;
  }
  crawl_info->set_pretransfer_time(pretransfer_time);

  double starttransfer_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_STARTTRANSFER_TIME,
                                    &starttransfer_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_STARTTRANSFER_TIME fail";
    return false;
  }
  crawl_info->set_starttransfer_time(starttransfer_time);

  double redirect_time;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_TIME,
                                    &redirect_time)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_TIME fail";
    return false;
  }
  crawl_info->set_redirect_time(redirect_time);

  crawl_info->set_redirect_times(redirect_count);

  double donwload_speed;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_SPEED_DOWNLOAD,
                                    &donwload_speed)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_SPEED_DOWNLOAD fail";
    return false;
  }
  crawl_info->set_donwload_speed(donwload_speed);

  int64 header_size;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_HEADER_SIZE,
                                    &header_size)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_HEADER_SIZE fail";
    return false;
  }
  crawl_info->set_header_bytes(header_size);

  double content_length_donwload;
  if (CURLE_OK != curl_easy_getinfo(curl_handle,
                                    CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                    &content_length_donwload)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONTENT_LENGTH_DOWNLOAD fail";
    return false;
  }
  crawl_info->set_content_length_donwload(content_length_donwload);

  char *content_type;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE,
                                    &content_type)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_CONTENT_TYPE fail";
    return false;
  }
  if (content_type != NULL) {
    crawl_info->set_content_type(content_type);
  }

  // update fail counter, set to 0
  crawl_info->set_update_fail_times(0);
  return true;
}
}  // namespace crawl
}  // namespace crwaler
