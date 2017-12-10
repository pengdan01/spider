#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>
#include <algorithm>

#include "crawler/util/text_handler.h"
#include "base/common/base.h"
#include "base/encoding/cescape.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/file/file_util.h"
#include "web/url/gurl.h"
#include "extend/regexp/re3/re3.h"
#include "extend/regexp/re2/re2.h"

DEFINE_string(regexp_file, "", "the file contains regexp");
DEFINE_string(content_file, "", "the file contains text");

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "test regexp");

  std::string regexp, page;
  CHECK(base::file_util::ReadFileToString(FLAGS_regexp_file, &regexp));
  base::TrimTrailingWhitespaces(&regexp);
  LOG(INFO) << "re is: '" << regexp << "'";

  CHECK(base::file_util::ReadFileToString(FLAGS_content_file, &page));
  LOG(INFO) << "page size: " << page.size();

  base::Slice input(page);
  std::vector<std::string> urls;
  std::string url, valid_url;
  std::string link("http://www.hao123.com");
  GURL base_url(link);
  const extend::re2::RE2 UrlPattern(regexp);
  // const extend::re3::Re3 UrlPattern(regexp);

  while (extend::re2::RE2::FindAndConsume(&input, UrlPattern, &url)) {
  // while (extend::re3::Re3::FindAndConsume(&input, UrlPattern, &url)) {
    LOG(INFO) << "found: " << url;
    LOG(INFO) << "input: " << input.size();
    GURL new_url = base_url.Resolve(url);
    if (new_url.is_valid()) {
      valid_url = new_url.spec();
      if (base::EndsWith(valid_url, ".exe", false)) {
        DVLOG(3) << "url ends with '.exe', ignored: " << valid_url;
        continue;
      }
      if (base::StartsWith(valid_url, "javascript:", false)) {
        DVLOG(3) << "url starts with 'javascript:', ignored: " << valid_url;
        continue;
      }
      // 去掉最后面的多余的 '#'
      if (valid_url[valid_url.size()-1] == '#') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      if (valid_url[valid_url.size()-1] == '/') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      urls.push_back(valid_url);
    } else {
      DVLOG(3) << "invalid url, ignored: " << new_url;
    }
  }
  if (urls.size() > 1) {  // sort and unique
    sort(urls.begin(), urls.end());
    urls.erase(unique(urls.begin(), urls.end()), urls.end());
  }

  for (int i = 0; i < (int)urls.size(); ++i) {
    LOG(INFO) << "url is: " << urls[i];
  }
  return 0;
}

