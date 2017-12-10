
#include <vector>
#include <string>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/strings/string_util.h"
#include "base/encoding/base64.h"
#include "base/encoding/cescape.h"
#include "base/encoding/url_encode.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

#include "web/url/gurl.h"
#include "web/url/url.h"
#include "web/url/url_util.h"

#include "log_analysis/common/log_common.h"

namespace log_analysis {

// 对 |url| 进行归一化处理, 并去除 |url| 的 query 部分
//
// 成功返回 true, |normed_url| 是归一化之后的结果
// 失败返回 false, |normed_url| == "", 只有当 url 非法时才会返回 false
bool abandon_url_query(const std::string& url, std::string* normed_url) {
  *normed_url = "";

  // 先把 raw url 转换成 click url
  std::string click_url;
  if (!web::RawToClickUrl(url, NULL, &click_url)) {
    DLOG(WARNING) << "Invalid url: " << url;
    return false;
  }

  // 用 gurl 获取成分
  GURL gurl(click_url);
  if (!gurl.is_valid()) {
    DLOG(WARNING) << "Invalid url: " << url;
    return false;
  }

  if (!gurl.has_query() && !gurl.has_ref()) {
    DLOG(INFO) << "url has no query and ref";
    *normed_url = click_url;
    return true;
  }

  // 去除 query 部分
  uint64 found = click_url.find_first_of("?");
  if (found == std::string::npos) {
    *normed_url = click_url;
    return true;
  }
  *normed_url = click_url.substr(0, found);
  return true;
}

bool Base64ToClickUrl(const std::string& text_base64, std::string* click_url) {
  std::string raw_url;
  if (!base::Base64Decode(text_base64, &raw_url)) {
    DLOG(WARNING) << "failed to decode base64: " << text_base64;
    return false;
  } else {
    if (!web::has_scheme(raw_url)) {
      raw_url = "http://" + raw_url;
    }
    if (!web::RawToClickUrl(raw_url, NULL, click_url)) {
      DLOG(WARNING) << "failed to convert to click url: " << raw_url;
      return false;
    } else {
      if (base::EndsWith(*click_url, "??L?", false) ||
          base::EndsWith(*click_url, "??W?", false) ||
          base::EndsWith(*click_url, "??U?", false) ||
          base::EndsWith(*click_url, "??Q?", false)) {
        // 这种是经过人工干预的 url, 已经损失了原始信息. 予以丢弃
        DLOG(WARNING) << "abnormal click url: " << *click_url;
        return false;
      }
    }
  }
  if (click_url->find('\t') != std::string::npos) {
    DLOG(WARNING) << "found TAB in url" << *click_url;
    return false;
  }
  return true;
}
}  // namespace log_analysis
