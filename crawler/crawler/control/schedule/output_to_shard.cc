// 把爬虫的输出划分为 shard
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_number_conversions.h"

#include "web/url/url.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/api/base.h"

DEFINE_int32(shard_num, 0, "shard number");

void Mapper() {
  // key: source url
  // value: crawled_resource protobuf
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::string output_key;
  while (task->NextKeyValue(&key, &value)) {
    // 反转 key  
    if (!web::ReverseUrl(key, &output_key, true)) {
      task->ReportAbnormalData("ReverseUrl failed in mapper", key, value);
    }
    task->Output(output_key, value);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::string output_key;
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      int shard_id;
      // 对 key 的 host 做反转
      if (!web::ReverseUrl(key, &output_key, true)) {
        task->ReportAbnormalData("ReverseUrl failed", key, value);
        continue;
      }
      // 获得 url 属于哪一个 shard
      if(!crawler::AssignShardId(output_key, FLAGS_shard_num, &shard_id, true)) {
        task->ReportAbnormalData("AssignShardId failed in reducer", key, value);
        continue;
      }
      // 输出到指定shard
      task->OutputWithFilePrefix(key, value, "shard-"+base::IntToString(shard_id));
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "crawler output to shard, maponly");
  if (FLAGS_shard_num == 0) {
    LOG(FATAL) << "please set FLAGS_shard_num";
  }
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false) << "it's a map only program";
  }
}
