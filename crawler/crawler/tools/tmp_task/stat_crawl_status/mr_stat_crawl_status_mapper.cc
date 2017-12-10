#include <iostream>
#include <string>
#include <unordered_map>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/url.h"
#include "crawler/proto/crawled_resource.pb.h"

DEFINE_int32(crawl_type_field_id, 4, "");
DEFINE_int32(success_field_id, 8, "");
DEFINE_int32(reason_field_id, 10, "");

DEFINE_int32(stat_type, 0, "0: Only Proxy; 1: Only Direct; 2: Both Proxy and Direct");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "parse html from HFfile");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;

  std::unordered_map<std::string, int64> ip_crawl_total;
  std::unordered_map<std::string, int64> ip_crawl_success;
  std::unordered_map<std::string, int64> ip_crawl_200;
  std::unordered_map<std::string, std::set<std::string>> ip_hosts;

  CHECK(FLAGS_stat_type == 0 || FLAGS_stat_type == 1 || FLAGS_stat_type == 2);

  // 网页数据输入的 status 记录 
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    int field_num = (int)tokens.size(); 
    if (field_num < 17) {
      LOG(ERROR) << "Record filed# error, value: " << value;
      continue;
    } 
    const std::string &proxy_or_direct = tokens[FLAGS_crawl_type_field_id];
    LOG_IF(FATAL, proxy_or_direct != "PROXY" && proxy_or_direct != "DIRECT")
        << "proxy or direct filed value invalid, value: " << proxy_or_direct;
    const std::string &ip = tokens[0];
    if ((FLAGS_stat_type == 0 && proxy_or_direct != "PROXY") ||
        (FLAGS_stat_type == 1 && proxy_or_direct != "DIRECT")) {
      LOG(ERROR) << "Proxy_or_direct: " << proxy_or_direct;
      continue;
    }
    ++ip_crawl_total[ip];
    GURL gurl(key);
    ip_hosts[ip].insert(gurl.host());

    const std::string &success =  tokens[FLAGS_success_field_id];
    LOG_IF(FATAL, success != "SUCCESS" && success != "FAILED")
        << "Invalid value for success field, value: " << success;
    if (success == "SUCCESS") {
      ++ip_crawl_success[ip];
    }
    const std::string &reason =  tokens[FLAGS_reason_field_id];
    if (reason == "200") {
      ++ip_crawl_200[ip];
    }
  }

  // output
 for (auto it = ip_crawl_total.begin(); it != ip_crawl_total.end(); ++it) {
  const std::string &ip = it->first;
  int64 total_num = it->second; 
  int64 curl_ok = ip_crawl_success[ip];
  int64 http200_ok = ip_crawl_200[ip];
  const std::set<std::string> &hosts = ip_hosts[ip];
  std::string top5_host;
  for (auto it2 = hosts.begin(); it2 != hosts.end(); ++it2) {
    if (top5_host.empty()) {
      top5_host = *it2;
    } else {
      top5_host = top5_host + "," + *it2;
    }
  }

  value = base::StringPrintf("%ld\t%ld\t%ld\t%s", total_num, curl_ok, http200_ok, top5_host.c_str());
  task->Output(ip, value);
 }
  return 0;
}
