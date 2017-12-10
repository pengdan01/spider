#include <string>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/counters.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/offline_analyze/offline_analyze_util.h"
#include "crawler/api/base.h"
#include "extend/simhash/html_simhash.h"

const int64 kLocalBufferSetSize = 1000;
DEFINE_string(white_list, "", "");
DEFINE_string(black_list, "", "");
DEFINE_int64_counter(record_counter, correct, 0, "the number of docs successfully procesed");
DEFINE_int64_counter(record_counter, wrong, 0, "the number of docs skipped");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  crawler::CrawledResource pb;
  simhash::HtmlSimHash html_simhash;

  LOG_IF(FATAL, !html_simhash.Init(FLAGS_white_list, FLAGS_black_list)) << "Failed: html_simhash.Init()";
  uint64 hash;

  // 网页数据输入的 key 为 source_url, value 为一个 proto buffer
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) {
      LOG(ERROR) << "Empty key";
      COUNTERS_record_counter__wrong.Increase(1);
      continue;
    }
    if (!pb.ParseFromString(value)) {
      LOG(ERROR) << "pb.ParseFromString(value) fail";
      COUNTERS_record_counter__wrong.Increase(1);
      continue;
    }
    hash = 0;
    if (pb.has_content_utf8() && !pb.content_utf8().empty()) {
      const std::string &content_utf8 = pb.content_utf8();
      if(!html_simhash.CalculateSimHash(content_utf8, pb.effective_url(), &hash)) {
        LOG(ERROR) << "Fail: CalculateSimHash(), url: " << pb.effective_url();
        hash = 0;
      }
    } else {
      COUNTERS_record_counter__wrong.Increase(1);
      LOG(ERROR) << "No utf8 content:\t" << pb.effective_url();
      hash = 0;
    }
    pb.set_simhash(hash);
    bool result = pb.SerializeToString(&value);
    if (result != true) {
      LOG(ERROR) << "Fail in pb.SerializeToString();";
      COUNTERS_record_counter__wrong.Increase(1);
      continue;
    }
    task->OutputWithFilePrefix(key, value, "data_");
    value = base::StringPrintf("%lu", hash);
    // task->OutputWithFilePrefix(pb.source_url(), value, "map_");
    task->OutputWithFilePrefix(key, value, "map_");
  }
  base::LogAllCounters();
  return 0;
}
