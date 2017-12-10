// 将拼接了 show clk 的原始数据转换为训练数据
#include <unordered_map>
#include <utility>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_number_conversions.h"

#include "log_analysis/common/log_common.h"

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;
  std::vector<std::string> fields;

  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    CHECK_EQ(fields.size(), 9u) << value;
    // from: sid, uid, timestamp, query, se_name, rank, res_url, pv_log_timestamp, show, click
    // to:   query, res_url, se_name, rank, show, click timestamp
    if (fields[3] != log_analysis::kBaidu
        && fields[3] != log_analysis::kGoogle
        && fields[3] != log_analysis::kBing) {
      continue;
    }
    task->Output(log_analysis::JoinFields(fields, "\t", "253"),
                 log_analysis::JoinFields(fields, "\t", "4781"));
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> fields;
  while (task->NextKey(&key)) {
    std::string search_engine_rank = "0";
    int search_engine_timestamp = 0;
    int64_t show = 0;
    int64_t click = 0;
    int64_t timestamp = 0;
    while (task->NextValue(&value)) {
      fields.clear();
      base::SplitString(value, "\t", &fields);
      CHECK_EQ(fields.size(), 4u) << value;

      // key:   query, res_url, se_name
      // value: rank, show, click timestamp
      timestamp = base::ParseInt64OrDie(fields[3]);
      if (timestamp > search_engine_timestamp) {
        search_engine_timestamp = timestamp;
        search_engine_rank = fields[0];
      }

      show += base::ParseInt64OrDie(fields[1]);
      click += base::ParseInt64OrDie(fields[2]);
    }

    task->Output(key, search_engine_rank + "\t"
                      + base::Int64ToString(show) + "\t" + base::Int64ToString(click));
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "transfer search log to train data");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
  return 0;
}
