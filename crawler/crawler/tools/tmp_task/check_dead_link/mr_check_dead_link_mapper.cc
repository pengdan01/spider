#include <iostream>
#include <string>
#include <unordered_map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/utf_codepage_conversions.h"
#include "web/url/url.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  auto task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;
  // key 为 hbase key, value 为 brief 的序列化
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    crawler2::Resource pb;
    if (!crawler2::HbaseDumpToResource(value.data(), value.size(), &pb)) {
      LOG(ERROR) << "pb.ParseFromString() fail, url: " << key;
      continue;
    }
    // 过滤掉 非 http 200 网页
    if (pb.brief().resource_type() != 1 || pb.brief().crawl_info().response_code() != 200) {
      continue;
    }
    const std::string &source_url  = pb.brief().source_url();
    const std::string &effective_url  = pb.brief().effective_url();
    int64 crawl_timestamp = pb.brief().crawl_info().crawl_timestamp();
    std::string out = base::StringPrintf("%s\t%ld", source_url.c_str(), crawl_timestamp);
    task->Output(effective_url, out);
  }
  return 0;
}
