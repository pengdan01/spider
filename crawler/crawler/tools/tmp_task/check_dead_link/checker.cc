#include <iostream>
#include <fstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include <curl/curl.h>

DEFINE_string(url_file, "crawler/tools/tmp_task/test_url", "");

DEFINE_string(dns_servers, "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25",
              "dns servers");
FILE *fp = fopen(".tmp_file", "w");
//
// XXX(pengdan): 给定 一个 url, 判断 Is norml link or not   
int CheckOneUrl(const std::string &url, bool using_head_method) {
  // 1. 初始化 easy handle
  CURL *eh = curl_easy_init();
  CHECK_NOTNULL(eh);
  // 2. 设置基本属性
  curl_easy_setopt(eh, CURLOPT_URL, url.c_str());
  curl_easy_setopt(eh, CURLOPT_TIMEOUT, 30);
  curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L);
  // XXX(pengdan): 1). 因为使用 effective url 进行 check, 所有不需要支持自动重定向;
  // 2). 百度, 文库 等站点, 当请求的站点不存在时, 会重定向倒一个错误页面, 返回 302
  //curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
  //curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 32L);
  curl_easy_setopt(eh, CURLOPT_COOKIEFILE, "");
  if (using_head_method== true) {
    curl_easy_setopt(eh, CURLOPT_NOBODY, 1L);
  } else {
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(eh, CURLOPT_HTTPGET, 1L);
  }
  // 3. 执行抓取
  CURLcode res = curl_easy_perform(eh);
  // 4. 分析抓取结果
  if (res != CURLE_OK) {
    curl_easy_cleanup(eh);
    return res;
  }
  int64 response_code;
  CHECK_EQ(CURLE_OK, curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &response_code));
  curl_easy_cleanup(eh);

  return (int)response_code;
}

int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "");
  std::ifstream myfin(FLAGS_url_file);
  CHECK(myfin.good());
  std::string line, url;

  while (getline(myfin, line)) {
    url = line;
    int ret = CheckOneUrl(url, true);
    if (ret == 405) {
      ret = CheckOneUrl(url, false);
    } 
    LOG(ERROR) << url << "\t" << ret << std::endl;
  }
  CHECK(myfin.eof());
  return 0;
}
