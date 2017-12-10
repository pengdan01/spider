#include <signal.h>
#include <sys/epoll.h>

#include "base/hash_function/city.h"
#include "base/common/scoped_ptr.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_util.h"
// #include "web/url/gurl.h"
#include "crawler/fetcher_simplify/fetcher.h"

namespace crawler2 {
const char* kCookiePath = "/dev/shm/curl_cookie.txt";
/*
void Fetcher::SetEasyHandlerForClientRedirect(CURL *eh, const UrlToFetch &url_to_fetch) {
  VLOG(2) << "set easy hanler for client redirected url: " << url_to_fetch.client_redirect_url;
  CHECK(!url_to_fetch.ip.empty());

  // 创建抓取过程中需要的各种临时对象 (private data)
  PrivateData *private_data = new PrivateData();
  private_data->url_to_fetch = url_to_fetch;
  private_data->start_time = base::GetTimestamp();
  const char *kAcceptLanguage = "Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3";
  private_data->request_header = curl_slist_append(private_data->request_header, kAcceptLanguage);
  GURL gurl(url_to_fetch.client_redirect_url);

  // 设置各种必要参数
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallbackBodyData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEDATA, private_data), CURLE_OK);

  // 不验证 SSL certificate
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &CallbackHeadData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERDATA, private_data), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_USERAGENT, options_.user_agent.c_str()), CURLE_OK);

  // body include head or not? we say no
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADER, 0L), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_URL, url_to_fetch.client_redirect_url.c_str()), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PRIVATE, private_data), CURLE_OK);

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
  if (!options_.dns_servers.empty()) {
    GURL gurl(url_to_fetch.client_redirect_url);
    uint64 hashcode = base::CityHash64(gurl.host().c_str(), gurl.host().size());
    const std::string &dns = options_.dns_servers[hashcode % options_.dns_servers.size()];
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, dns.c_str()), CURLE_OK);
    VLOG(4) << "set dns " << dns << " to url: "  << url_to_fetch.client_redirect_url;
  }

  // 支持 Https 协议
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);

  // support cookies
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEFILE, kCookiePath), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEJAR, kCookiePath), CURLE_OK);

  //  设置 refer
  if (!url_to_fetch.referer.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REFERER, url_to_fetch.referer.c_str()), CURLE_OK);
  }

  // 设置 header Accept-Language
  if (private_data->request_header != NULL) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HTTPHEADER, private_data->request_header), CURLE_OK);
  }

  // 设置代理
  if (private_data->url_to_fetch.use_proxy) {
    CHECK(!options_.proxy_servers.empty());
    const std::string &proxy_server = SelectBestProxy();
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROXY, proxy_server.c_str()), CURLE_OK);
    private_data->proxy_server_used = proxy_server;
  }
}
*/
void Fetcher::SetEasyHandler(CURL *eh, const UrlToFetch &url_to_fetch) {
  VLOG(2) << "set easy hanler for url: " << url_to_fetch.url;

  // 创建抓取过程中需要的各种临时对象 (private data)
  PrivateData *private_data = new PrivateData();
  private_data->url_to_fetch = url_to_fetch;
  private_data->start_time = base::GetTimestamp();
  const char *kAcceptLanguage = "Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3";
  private_data->request_header = curl_slist_append(private_data->request_header, kAcceptLanguage);
  // GURL gurl(url_to_fetch.url);

  // 设置各种必要参数
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, &CallbackBodyData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_WRITEDATA, private_data), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, &CallbackHeadData), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADERDATA, private_data), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_USERAGENT, options_.user_agent.c_str()), CURLE_OK);

  // 不验证 SSL certificate
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L), CURLE_OK);

  // body include head or not? we say no
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HEADER, 0L), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_URL, url_to_fetch.url.c_str()), CURLE_OK);

  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PRIVATE, private_data), CURLE_OK);

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
  //if (!options_.dns_servers.empty()) {
  //  GURL gurl(url_to_fetch.url);
  //  uint64 hashcode = base::CityHash64(gurl.host().c_str(), gurl.host().size());
  //  const std::string &dns = options_.dns_servers[hashcode % options_.dns_servers.size()];
  //  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_DNS_SERVERS, dns.c_str()), CURLE_OK);
  //  VLOG(4) << "set dns " << dns << " to url: "  << url_to_fetch.url;
  //}

  // XXX(pengdan): 同时支持 http https 2013-04-17
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS), CURLE_OK);

  // support cookies, 不然 蘑菇街 等首页抓取不能
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEFILE, kCookiePath), CURLE_OK);
  CHECK_EQ(curl_easy_setopt(eh, CURLOPT_COOKIEJAR, kCookiePath), CURLE_OK);

  //  设置 refer
  if (!url_to_fetch.referer.empty()) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_REFERER, url_to_fetch.referer.c_str()), CURLE_OK);
  }

  // 设置 header Accept-Language
  if (private_data->request_header != NULL) {
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_HTTPHEADER, private_data->request_header), CURLE_OK);
  }

  // 策略: 
  // 1. 如果 url_to_fetch 配置了 use_proxy(true), 则使用 SOCKS5 代理抓取; 反之, 直接抓取
  //
  if (private_data->url_to_fetch.use_proxy) {
    CHECK(!options_.proxy_servers.empty());
    const std::string &proxy_server = SelectBestProxy();
    CHECK_EQ(curl_easy_setopt(eh, CURLOPT_PROXY, proxy_server.c_str()), CURLE_OK);
    private_data->proxy_server_used = proxy_server;
  }
}

bool Fetcher::ExtractInfoFromCurl(CURLMsg *msg, const PrivateData *private_data, UrlFetched *url_fetched) {
  CURL *curl_handle = msg->easy_handle;

  // 设置 url fected 的简单成员
  // ===============================================
  // 1. 先设置 url_fetched 的简单成员 (除了 proto Resource 以外的成员)
  url_fetched->url_to_fetch = private_data->url_to_fetch;
  url_fetched->start_time = private_data->start_time;
  url_fetched->end_time = base::GetTimestamp();
  url_fetched->proxy_server = private_data->proxy_server_used;

  if (msg->data.result != 0 && msg->data.result != 23) {
    // 若抓取失败, 则仅设置少量结果, 即返回
    url_fetched->success = false;
    url_fetched->ret_code = msg->data.result;
    url_fetched->reason = curl_easy_strerror(msg->data.result);
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
    url_fetched->success = false;
    url_fetched->ret_code = msg->data.result;
    url_fetched->reason = curl_easy_strerror(msg->data.result);
    return true;
  }

  // 抓取成功, 才会继续提取信息
  url_fetched->success = true;
  url_fetched->ret_code = http_response_code;
  url_fetched->reason = base::StringPrintf("%ld", http_response_code);

  int64 redirect_count;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_COUNT, &redirect_count)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_REDIRECT_COUNT fail";
    return false;
  }
  url_fetched->redir_times = redirect_count;

  char *raw_effective_url;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &raw_effective_url)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_EFFECTIVE_URL fail";
    return false;
  }
  LOG_IF(FATAL, raw_effective_url == NULL) << "effective url is NULL pointer";

  // XXX huangboxiang, 被 client redirect 词表修改的 url
  std::string hack_raw_effective_url = raw_effective_url;
  if (!url_fetched->url_to_fetch.client_redirect_url.empty()) {
    hack_raw_effective_url = url_fetched->url_to_fetch.url;
  }

  url_fetched->effective_url = hack_raw_effective_url;
  url_fetched->file_bytes = private_data->body.size();

  double download_bytes;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &download_bytes)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_SIZE_DOWNLOAD fail";
    return false;
  }
  url_fetched->download_bytes = (int64)download_bytes;

  if (msg->data.result == 23) {
    url_fetched->truncated = true;
  } else {
    url_fetched->truncated = false;
  }

  // TODO(suhua): 解析 http response header, 识别下载下来的资源的真实 resource type, 并修改 proto buffer

  // 设置 crawled resource
  // ====================================================
  // 3. 若 抓取成功, 则设置 url_fetched 的成员 proto Resource
  // get head  & body content
  url_fetched->header_raw = private_data->head;
  url_fetched->page_raw = private_data->body;

  int64 response_code;
  if (CURLE_OK != curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code)) {
    LOG(ERROR) << "Call curl_easy_getinfo() get CURLINFO_RESPONSE_CODE fail";
    return false;
  }
  url_fetched->http_response_code = response_code;

  return true;
}

}  // namespace crawler2

