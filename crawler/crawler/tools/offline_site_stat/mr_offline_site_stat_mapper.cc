#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_tokenize.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/gurl.h"

DEFINE_string(site_list, "", "contains the web sites we will stat");
const char flag1 = 'A';
const char flag2 = 'B';
const int64 kMaxLocalCacheSize = 800000;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "offline web site stat tool");
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);
  CHECK(!FLAGS_site_list.empty());

  std::vector<std::string> sites;
  base::Tokenize(FLAGS_site_list, ",", &sites);
  DLOG(INFO) << "site list: " << FLAGS_site_list << std::endl;

  std::string key;
  std::string value;
  std::string source_url;
  std::string link_type;
  std::map<std::string, int64> buffers;
  std::map<std::string, int64> hosts;
  std::map<std::string, int64>::const_iterator it;
  std::vector<std::string> tokens;
  // link 数据输入格式，14 个 fields：（自解释）
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::Tokenize(value, "\t", &tokens);
    source_url = key;
    link_type = tokens[1];
    LOG_IF(ERROR, link_type != "1" && link_type != "2" && link_type != "3")
        << "Invalid type: " << link_type << ", input record: " << key + "\t" + value;
    if (link_type != "1") {
      continue;  // 只统计网页类型 css/image 暂时不统计
    }
    for (int i = 0; i < (int)sites.size(); ++i) {
      if (base::MatchPattern(source_url, "*." + sites[i] + ".*")) {
        buffers[sites[i]]++;
      }
    }
    GURL gurl(source_url);
    if (gurl.is_valid()) {
      hosts[gurl.host()]++;
      if ((int64)hosts.size() > kMaxLocalCacheSize) {
        it = hosts.begin();
        while (it != hosts.end()) {
          std::string out_value = base::StringPrintf("%c%ld", flag2, it->second);
          task->Output(it->first, out_value);
          ++it;
        }
        hosts.clear();
      }
    }
  }
  it = buffers.begin();
  while (it != buffers.end()) {
    std::string out_value = base::StringPrintf("%c%ld", flag1, it->second);
    task->Output(it->first, out_value);
    ++it;
  }
  buffers.clear();
  it = hosts.begin();
  while (it != hosts.end()) {
    std::string out_value = base::StringPrintf("%c%ld", flag2, it->second);
    task->Output(it->first, out_value);
    ++it;
  }
  hosts.clear();
  return 0;
}
