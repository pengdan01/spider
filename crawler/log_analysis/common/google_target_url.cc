#include <vector>
#include <string>
#include <map>

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
#include "web/url/url.h"
#include "web/url/url_util.h"
#include "base/time/time.h"

#include "log_analysis/common/log_common.h"

namespace log_analysis {

static const char* kGoogleHost = "www.google.";
static const char* kGooglePath = "/url";
// static const char* kGoogleTargetUrlKey = "url";

bool ParseGoogleTargetUrl(const std::string& orig_url, std::string* target_url) {
  *target_url = "";
  const std::string& url = orig_url;

  GURL gurl(url);
  if (!gurl.has_host() || !gurl.has_path() || !gurl.has_query()) {
    DLOG(INFO) << "host/path/query incomplete in url: " << url;
    return false;
  }
  std::string host = gurl.host();
  std::string path = gurl.path();
  size_t found = host.find(kGoogleHost);
  if (found == std::string::npos ||
      path != kGooglePath) {
    DLOG(INFO) << "not a google result" << url;
    return false;
  }

  // parse raw_url
  std::vector<std::pair<std::string, std::string>> kv_pair;
  if (!web::GetQueryKeyValuePairs(url, false, &kv_pair)) {
    DLOG(INFO) << "failed to parse query" << url;
    return false;
  }

  std::multimap<std::string, std::string> kv_map;
  for (auto it = kv_pair.begin(); it != kv_pair.end(); it++) {
    kv_map.insert(std::make_pair(it->first, it->second));
  }
  auto jt = kv_map.find("sa");
  if (jt == kv_map.end()) {
    DLOG(INFO) << "failed to find key sa" << url;
    return false;
  }
  const std::string& value = jt->second;
  auto it = kv_map.end();
  if (value == "U") {
    it = kv_map.find("q");
  } else if (value == "t") {
    it = kv_map.find("url");
  } else if (value == "X") {
    it = kv_map.find("url");
  }

  if (it == kv_map.end()) {
    DLOG(INFO) << "failed to find key url in sa=t" << url;
    return false;
  }

  std::string raw_url = it->second;
  if (!web::has_scheme(raw_url)) {
    if (!base::DecodeUrlComponent(raw_url.c_str(), target_url)) {
      LOG(ERROR) << "failed to decode query: " << raw_url;
      return false;
    }
  } else {
    *target_url = raw_url;
  }
  GURL lg(*target_url);
  if (!lg.is_valid()) {
    *target_url = "";
    return false;
  }

  return true;
}
}
