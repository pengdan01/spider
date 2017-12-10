#include <string>
#include <sstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "crawler/proto/crawled_resource.pb.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  crawler::CrawledResource info;

  // 网页数据输入的 key 为 source_url 的反转, value 为一个 proto buffer
  while (task->NextKeyValue(&key, &value)) {
    LOG_IF(FATAL, key.empty() || value.empty()) << "key or value is empty";
    if (!info.ParseFromString(value)) {
      LOG(ERROR) << "info.ParseFromString(value) fail";
      continue;
    }
    key =  info.source_url();
    task->Output(key, "");
  }

  return 0;
}
