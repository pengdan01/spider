#pragma once

#include <vector>
#include <string>
#include <utility>

#include "base/common/basic_types.h"
namespace crawler {
// this function will filter out pages ends with ".exe"...
// this function will filter out pages starts with "javascript mailto"
// the urls end with '#' will remove the last '#'
// the urls end with '/' will remove the last '/'
void url_preprocess(std::vector<std::string> *urls,
                    const std::vector<std::pair<std::string, std::string> > &anchors,
                    const std::vector<std::string> &ignore_page_postfix,
                    const std::vector<std::string> &ignore_page_prefix);

// In this struct , |value| is a serialized string of protocal buffer
// |timestamp| is the timestamp when this page is crawled
struct ProtocalBufferWithTimestamp {
  std::string value;
  std::string rurl;
  int64 timestamp;
  ProtocalBufferWithTimestamp(): timestamp(0) {}
  ProtocalBufferWithTimestamp(const std::string &value, const std::string &rurl,
                              int64 timestamp): value(value), rurl(rurl), timestamp(timestamp) {}
};

}  // namespace crawler
