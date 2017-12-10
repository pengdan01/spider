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

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;

  // 网页数据输入的 dai zhua qu 7u 
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    int field_num = (int)tokens.size(); 
    CHECK_EQ(field_num, 17);
    const std::string &proxy_or_direct = tokens[4];
    LOG_IF(FATAL, proxy_or_direct != "PROXY" && proxy_or_direct != "DIRECT")
        << "proxy or direct filed value invalid, value: " << proxy_or_direct;
    if (proxy_or_direct == "DIRECT") { 
      task->Output(key, "");
    }
  }
  return 0;
}
