#pragma once

#include <string>

#include "base/common/basic_types.h"

namespace crawler2 {
namespace crawl {

// 状态码定义
// 前 8 位用于区别类型
// 1 代表 HTTP ResponseCode
// 2 代表 CURL 失败
// 3 代表 网络原因失败
// 4 代表代理原因
class StatusCode {
 public:
  enum {
    kHTTPOK = 200 | 0x00000000,
    kHTTPNotFound = 400 | 0x00000000,
  };

  enum {
    kHTTPStatus = 0x10000000,
    kCURLStatus = 0x20000000,
  };

  static uint32 GetRetCodeFromCURLError(int rc) {
    return rc | kCURLStatus;
  }

  static uint32 GetRetCodeFromHTTP(int rc) {
    return rc | kHTTPStatus;
  }

  std::string GetErrorDescription(uint32 ret_code) {
    if (ret_code & kCURLStatus) {
      return curl_easy_strerror((CURLcode)(ret_code & (0x0000FFFF)));
    } else {
      return "";
    }
  }
};
}  // namespace crawl
}  // namespace crawler
