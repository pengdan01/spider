// 把 url 大于 10w 的 ip 区分出来
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <fstream>
#include <iostream>
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
#include "web/url/url.h"
#include "crawler/exchange/task_data.h"
DEFINE_int64(ip_url_threshold, 100000, "ip url threshold");
DEFINE_bool(force_proxy, false, "force proxy");
DEFINE_string(proxy_white_list_path, "", "white list of proxy");

void ReadProxyWhiteList(std::set<std::string> *hosts) {
  hosts->clear();
  std::string read_buffer;
  std::ifstream fin(FLAGS_proxy_white_list_path);
  if (!fin) {
    LOG(FATAL) << "open " << FLAGS_proxy_white_list_path << " failed";
  }
  while (std::getline(fin, read_buffer)) {
    hosts->insert(read_buffer);
  }
  fin.close();
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    const std::string &ip=fields[0];
    // output: ip \t url
    task->Output(ip , key+"\t"+value);
  }
}

std::string AddPayload(std::string value, std::string payload) {
  std::string output_value;
  std::vector<std::string> fields;
  base::SplitString(value, "\t", &fields);
  CHECK_EQ(crawler::kTaskItemNum-1, fields.size());
  fields[4] = payload;
//   output_value = fields[0] + "\t" + fields[1] + "\t" + fields[2] + "\t" 
//                + fields[3] + "\t" + fields[4] + "\t" + fields[5];
  output_value = ::base::JoinStrings(fields, "\t");
  return output_value;
}

void Reducer() {
  std::set<std::string> domains;
  ReadProxyWhiteList(&domains);
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::vector<std::string> ip_count;
  std::string output_file_prefix;
  std::string payload;
  std::string tld;
  std::string domain;
  std::string subdomain;
  int64_t total_url_num = 0;
  while (task->NextKey(&key)) {
    ip_count.clear();
    total_url_num = 0;
    output_file_prefix = "crawler_task";
    payload = "DIRECT";
    if (FLAGS_force_proxy == true) {
      payload = "PROXY";
    }
    while (task->NextValue(&value)) {
      ++total_url_num;
      if (total_url_num >= FLAGS_ip_url_threshold) {
        // 输出
        size_t tab_pos = value.find("\t");
        if (tab_pos == std::string::npos) {
          LOG(FATAL) << "value format error: " << value;
        }
        std::string output_key = value.substr(0, tab_pos);
        std::string output_value = value.substr(tab_pos+1, value.length() - tab_pos - 1);
        output_file_prefix = "larger_ip_url-crawler_task";
        payload = "PROXY";
        task->OutputWithFilePrefix(output_key, AddPayload(output_value, payload), output_file_prefix);
      } else {
        ip_count.push_back(value);
      }
    }

    for (size_t i = 0; i < ip_count.size(); ++i) {
      size_t tab_pos = ip_count[i].find("\t");
      if (tab_pos == std::string::npos) {
        LOG(FATAL) << "value format error: " << ip_count[i];
      }
      std::string output_key = ip_count[i].substr(0, tab_pos);
      std::string output_value = ip_count[i].substr(tab_pos+1, ip_count[i].length() - tab_pos - 1);
      GURL url(output_key);
      if (url.is_valid() && web::ParseHost(url.host(), &tld, &domain, &subdomain)) {
        if (domains.count(domain) != 0) {
          payload = "PROXY";
        }
      }
      task->OutputWithFilePrefix(output_key, AddPayload(output_value, payload), output_file_prefix);
    }
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
