#include "crawler/offline_analyze/offline_analyze_util.h"

#include <utility>

#include "base/strings/string_util.h"

namespace crawler {

void url_preprocess(std::vector<std::string> *urls,
                    const std::vector<std::pair<std::string, std::string> > &anchors,
                    const std::vector<std::string> &ignore_page_postfix,
                    const std::vector<std::string> &ignore_page_prefix) {
  CHECK_NOTNULL(urls);
  if (anchors.empty()) return;
  std::string url;
  bool flag;
  std::vector<std::pair<std::string, std::string> >::const_iterator it = anchors.begin();
  for (; it != anchors.end(); ++it) {
    flag = false;
    url = it->first;
    base::TrimWhitespaces(&url);
    if (url.empty()) continue;
    for (int i = 0; i < (int)ignore_page_postfix.size(); ++i) {
      if (base::EndsWith(url, ignore_page_postfix[i], false)) {
        flag = true;
        break;
      }
    }
    for (int i = 0; i < (int)ignore_page_prefix.size() && !flag; ++i) {
      if (base::StartsWith(url, ignore_page_prefix[i], false)) {
        flag = true;
        break;
      }
    }
    if (!flag) {
      if (url[url.size()-1] == '#') {
        url = url.substr(0, url.size()-1);
      }
      if (url[url.size()-1] == '/') {
        url = url.substr(0, url.size()-1);
      }
      urls->push_back(url);
    }
  }
  return;
}
}  // namespace crawler
