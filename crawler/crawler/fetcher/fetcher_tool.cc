#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/common/closure.h"
#include "base/time/timestamp.h"
#include "base/strings/string_util.h"
#include "base/thread/thread_pool.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/util/dns_resolve.h"
#include "crawler/util/ip_scope.h"

DEFINE_string(url, "www.baidu.com", "the url.");

void SetOption();
void GetInfo();

int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "Fetcher tool");

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  LOG_IF(FATAL, curl == NULL) << "curl_easy_init() fail";

  curl_easy_setopt(curl, CURLOPT_URL, FLAGS_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    LOG(ERROR) << "Err code: " << res  << ", msg: "
               << curl_easy_strerror(res) << ", url: " << FLAGS_url;
    return -1;
  }

  int64 response_code;
  if (CURLE_OK != curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code)) {
    LOG(ERROR) << "Failed to curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code)";
    return -1;
  }
  LOG(INFO) <<"Http Response Code:" << response_code << " for url: " << FLAGS_url;

  curl_easy_cleanup(curl);
  return 0;
}

