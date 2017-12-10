#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/api/base.h"

const int64 kMaxLocalCacheSize = 1000;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "build page index dict");
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::map<std::string, std::set<std::string> > buffers;
  std::map<std::string, std::set<std::string> >::const_iterator it;
  std::vector<std::string> tokens;
  // pv log 数据输入格式，10 个 fields
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    int num = tokens.size();
    if (num != 9) {
      LOG(INFO) << "Invalid pv log record value, part# should be 9 ,but is: " << num << ", record: "
                << key  + "\t" + value;
      continue;
    }
    std::string url(tokens[1]);
    std::string refer(tokens[2]);
    if (!crawler::NormalizeUrl(url, &url)) {
      LOG(WARNING) << "Failed in Normalize and ignore Url: " << url;
      continue;
    }
    if (!crawler::NormalizeUrl(refer, &refer)) {
      LOG(WARNING) << "Failed in Normalize and ignore Url: " << refer;
      continue;
    }
    buffers[refer].insert(url);
    if ((int)buffers.size() > kMaxLocalCacheSize) {
      for (it = buffers.begin(); it != buffers.end(); ++it) {
        const std::string &key = (*it).first;
        const std::set<std::string> &url_set = (*it).second;
        for (std::set<std::string>::const_iterator it2 = url_set.begin(); it2 != url_set.end(); ++it2) {
          task->Output(key, *it2);
        }
      }
      buffers.clear();
    }
  }
  if (buffers.size() > 0) {
    for (it = buffers.begin(); it != buffers.end(); ++it) {
      const std::string &key = (*it).first;
      const std::set<std::string> &url_set = (*it).second;
      for (std::set<std::string>::const_iterator it2 = url_set.begin(); it2 != url_set.end(); ++it2) {
        task->Output(key, *it2);
      }
    }
    buffers.clear();
  }
  return 0;
}
