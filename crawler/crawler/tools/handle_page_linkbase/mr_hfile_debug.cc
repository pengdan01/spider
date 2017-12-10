#include <string>
#include <sstream>
#include <map>

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

  // 网页数据输入的 key 为 source_url, value 为一个 proto buffer
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty()) continue;
    if (!info.ParseFromString(value)) {
      LOG(ERROR) << "info.ParseFromString(value) fail";
      continue;
    }
    task->Output(key, info.Utf8DebugString());
  }

  return 0;
}
