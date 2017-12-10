#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/fast_hash.h"
#include "base/random/pseudo_random.h"
#include "base/time/timestamp.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

// url \t url_type \t score \t from \t refer_url \t refer_from
const uint32 kSelect_Value_Output_FieldNum = 5;
// Updater 模块输出的 记录的 value 是 四个字段, 整个记录格式:
// url \t url_type \t score \t from \t last_modified_time
const uint32 kUpdate_Value_Output_FieldNum = 4;
const uint32 kStat_Value_Output_FieldNum = 1;

DEFINE_int32(max_currency_access_thread, 3, "The MAX number of threads that will access the same web host "
             "concurrently. Usually, it will not reach this peak value");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "Dispatcher Compress Control");
  base::mapreduce::MapTask * task = base::mapreduce::GetMapTask();
  CHECK_GT(FLAGS_max_currency_access_thread, 0);
  int reducer_num = task->GetReducerNum();
  LOG(INFO) << "REDUCER_NUM: " << reducer_num;
  base::PseudoRandom pr(base::GetTimestamp());

  std::string key;
  std::string value;
  std::vector<std::string> tokens;
  uint64 id = 0;
  uint64 filter_cnt = 0;
  uint64 offset = 0;
  char flag;
  while (task->NextKeyValue(&key, &value)) {
    LOG_IF(FATAL, key.empty() || value.empty()) << "key or value is empty";
    tokens.clear();
    size_t num;
    base::SplitString(value, "\t", &tokens);
    num = tokens.size();
    switch (num) {
      // selector 输出记录格式： url \t url_type \t score \t from \t refer_url \t refer_from
      case kSelect_Value_Output_FieldNum: {
        flag = 'B';
        GURL url(key);
        if (!url.is_valid())  {
          LOG(ERROR) << "url invalid, url: " << key;
          ++filter_cnt;
          continue;
        }
        const std::string &host = url.host();
        id = base::CityHash64(host.c_str(), host.size()) % reducer_num;
        offset = pr.GetInt(0, FLAGS_max_currency_access_thread - 1);
        // selector 记录输出到 host 可能分布到的某一个 bucket.
        // 目的：负载均衡 避免个别 bucket 特别大的情况
        task->OutputWithReducerID(host + flag, key + '\t' + value, (id + offset) % reducer_num);
        break;
      }
      // updater  输出记录格式： url \t url_type \t score \t from \t last_modified_time
      case kUpdate_Value_Output_FieldNum: {
        flag = 'B';
        GURL url(key);
        if (!url.is_valid()) continue;
        const std::string &host = url.host();
        id = base::CityHash64(host.c_str(), host.size()) % reducer_num;
        offset = pr.GetInt(0, FLAGS_max_currency_access_thread - 1);
        // selector 记录输出到 host 可能分布到的某一个 bucket.
        // 目的：负载均衡 避免个别 bucket 特别大的情况
        task->OutputWithReducerID(host + flag, key + '\t' + value, (id + offset) % reducer_num);
        break;
      }
      // stats 输出记录格式： host \t accessed_times
      case kStat_Value_Output_FieldNum: {
        flag = 'A';
        const std::string &host = key;
        id = base::CityHash64(host.c_str(), host.size()) % reducer_num;
        // stat 记录输出到 host 可能分布到的所有的 bucket.
        // 目的：负载均衡 避免个别 bucket 特别大的情况
        for (int i = 0; i < FLAGS_max_currency_access_thread; ++i) {
          task->OutputWithReducerID(host + flag, value, (id + i) % reducer_num);
        }
        break;
      }
      default:
        LOG(ERROR) << "Format error, record field#: " << num << ", record: " << key +"\t" + value;
        continue;
    }
  }
  LOG(INFO) << "Filter url#: " << filter_cnt;
  return 0;
}
