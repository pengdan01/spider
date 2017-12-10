#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/api/base.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/offline_analyze/offline_analyze_util.h"

DEFINE_int32(shard_num, 0, "The number of shard");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);
  LOG_IF(FATAL, FLAGS_shard_num <= 0) << "Invalid shard number: " << FLAGS_shard_num;

  std::string key;
  std::string value;
  std::string out_value;
  uint64 out_hash = 0;
  int out_shard;

  crawler::CrawledResource pb;
  int64 timestamp = 0;
  // 网页数据输入的 key 为 source_url, value 为一个序列化的 protocal buffer 的 string
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    if (!crawler::AssignShardId(key, FLAGS_shard_num, &out_shard, false)) {
      LOG(ERROR) << "Failed in AssignShardId, for url: " << key;
      continue;
    }
    timestamp = 0;
    out_value = "";
    while (task->NextValue(&value)) {
      if (!pb.ParseFromString(value)) {
        LOG(ERROR) << "pb.ParseFromString(value) fail";
        continue;
      }
      int64 tmp_timestamp = pb.crawler_info().crawler_timestamp();
      if (tmp_timestamp > timestamp) {
        timestamp = tmp_timestamp;
        out_value = value;
        out_hash = pb.simhash();
      }
    }
    if (!out_value.empty()) {
      std::string prefix = base::StringPrintf("%03d_", out_shard);
      std::string hash_str= base::StringPrintf("%lu", out_hash);
      task->OutputWithFilePrefix(key, out_value, prefix);
      task->OutputWithFilePrefix(key, hash_str, "map_");
    }
  }
  return 0;
}
