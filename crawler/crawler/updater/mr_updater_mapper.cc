#include <string>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/cescape.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/fast_hash.h"
#include "crawler/api/base.h"
#include "crawler/updater/updater_util.h"

const int kLinkBaseFiledNum = 15;

DEFINE_int32(start_shard_id, 0, "");
DEFINE_int32(end_shard_id, 0, "");
DEFINE_int32(shard_num, 128, "");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "mr_updater_mapper");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;

  int resource_type;
  std::string http_header;
  std::string time;
  std::string out;

  bool status;
  // linkbase record: 14 fields
  // source_url \t effective_url \t type \t timestamp ...
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    int num = base::Tokenize(value, "\t", &tokens);
    if (num < kLinkBaseFiledNum - 1) {
      LOG(ERROR) << "Invalid record in linkbase: filed# should >= 14, url is: " << key
                 << ", field# : " << num + 1;
      continue;
    }
    resource_type = tokens[1][0] - '0';
    // 只对网页进行更新, 跳过 image/css
    if (resource_type != 1) continue;

    // 计算 shard id
    int shard_id;
    status = crawler::AssignShardId(key, FLAGS_shard_num, &shard_id, false);
    if (status == false) {
      LOG(WARNING) << "AssignShardId fail, url: " << key << ", shard_num: " << FLAGS_shard_num;
      continue;
    }
    // 检查当前 URL 是否属于给定区间
    if (!(shard_id >= FLAGS_start_shard_id && shard_id < FLAGS_end_shard_id)) continue;

    const std::string &score = tokens[3];
    const std::string &http_header = tokens[11];
    const std::string &timestamp = tokens[2];
    status = crawler::ExtractParameterValueFromHeader(http_header, "Last-Modified", &time);
    if (status == false) {
      status = crawler::ExtractParameterValueFromHeader(http_header, "Date", &time);
      if (status == false) {
        LOG(WARNING) << "Extrct 'Last-Modified' and 'Date' fail, Ignore url:  " << key;
        continue;
      }
    }
    out = base::StringPrintf("%d\t%s\t%s\t%s", resource_type, timestamp.c_str(), score.c_str(), time.c_str());
    task->Output(key, out);
  }
  return 0;
}
