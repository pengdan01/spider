#include "crawler/crawl/crawl_util.h"

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/encoding/url_encode.h"
#include "web/url/url.h"

namespace crawler2 {
namespace crawl {
bool IsAjaxUrl(const std::string &url) {
  if (url.empty()) {
    return false;
  }
  std::string::size_type pos = url.find("#!");
  return (pos != std::string::npos);
}

void TransformAjaxUrl(const std::string &url, std::string *new_url) {
  static const char *escaped_fragment = "_escaped_fragment_";
  CHECK_NOTNULL(new_url);
  CHECK(IsAjaxUrl(url)) << "url is not AJAX url, url: " << url;

  std::string::size_type pos = url.find("#!");
  std::string part0 = url.substr(0, pos);
  std::string part1 = url.substr(pos+2);
  if (part1.empty()) {
    *new_url = part0;
  } else {
    *new_url = part0 + "?" + escaped_fragment + "=" + part1;
  }
}
}
}
