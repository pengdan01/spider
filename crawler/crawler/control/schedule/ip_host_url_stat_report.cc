#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"
#include "crawler/exchange/task_data.h"
void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  std::map<std::string, uint64_t> url_count;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    CHECK_EQ(crawler::kTaskItemNum-1, fields.size());
    const std::string &ip=fields[0];
    // output: ip \t url
    task->Output(ip , key);
  }
}

bool host_url_cmp(std::pair<std::string, size_t> left, std::pair<std::string, size_t> right) {
  return left.second > right.second;
}

void HostSort(const std::map<std::string, int64> &host_url,
              std::vector<std::string> *host_order) {
  host_order->clear();
  std::vector<std::pair<std::string, size_t>> host_count;
  auto it = host_url.begin();
  for (; it != host_url.end(); ++it) {
    host_count.push_back(make_pair(it->first, it->second));
  }
  std::sort(host_count.begin(), host_count.end(), host_url_cmp);
  auto it2 = host_count.begin();
  for (; it2 != host_count.end(); ++it2) {
    host_order->push_back(it2->first);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::map<std::string, int64> host_count; // 一个 ip 下　host　的个数
  std::map<std::string, std::vector<std::string>> host_url; // 一个 host 下的 url
  int64_t total_url_num = 0;
  while (task->NextKey(&key)) {
    total_url_num = 0;
    host_count.clear();
    host_url.clear();
    while (task->NextValue(&value)) {
      ++total_url_num;
      GURL url(value);
      const std::string &host = url.host();
      host_count[host]++;
      if (host_url[host].size() < 5UL) {
        host_url[host].push_back(value);
      }
    }
    // 输出: ip, ip 对应的 url 个数， ip 对应的 host 个数， host_1, host_1 对应的 3 个url， host_2, host_2 对应的 3 个 url ...
    std::string output_value;
    std::vector<std::string> host_order;
    output_value += base::Int64ToString(total_url_num) + "\t"; // url 个数
    output_value += base::Int64ToString(host_count.size()) + "\t"; // host 个数
    HostSort(host_count, &host_order);
    for (size_t host_i = 0; host_i < host_order.size(); ++host_i) {
      if (host_i == 3) {
        break;
      }
      output_value += host_order[host_i] + "\t"; // host
      output_value += base::Int64ToString(host_count[host_order[host_i]]) + "\t"; // host 对应的 url 个数
    }
    task->Output(key, output_value);
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "host stat");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
