#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    const std::string& mid = flds[0];
    const std::string& time = flds[1];
    int32 reduce_id = base::CityHash64(mid.c_str(), mid.size()) % reducer_count;
    task->OutputWithReducerID(mid + "\t" + time,
                              log_analysis::JoinFields(flds, "\t", "23456"), reduce_id);
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  while (task->NextKey(&key)) {
    std::string value;
    while (task->NextValue(&value)) {
      task->Output(key, value);
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "pv combine");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
