#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/url.h"
#include "crawler/proto2/resource.pb.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "parse html from HFfile");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  crawler2::Resource pb;

  // 网页数据输入的 key 为 source_url, value 为一个 proto buffer
  while (task->NextKey(&key)) {
    while(task->NextValue(&value)) {
      pb.Clear();
      if (!pb.ParseFromString(value)) {
        LOG(ERROR) << "pb.ParseFromString(value) fail";
        continue;
      }
      task->Output(key, pb.DebugString());
    }
  }
  return 0;
}
