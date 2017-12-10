// 把拼接好的 search log 转换成 原始 search log 的格式，即以 search_id 为 key, 按 rank 排序
#include <stdlib.h>
#include <time.h>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_number_conversions.h"

#include "log_analysis/common/log_common.h"

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_key;
  int rank;
  char str_rank[2];
  str_rank[1] = '\0';
  while (task->NextKeyValue(&key, &value)) {
    flds.clear();
    base::SplitString(value, "\t", &flds);
    // change rank to int
    base::StringToInt(flds[4], &rank);
    str_rank[0] = rank;
    output_key = key+"\t"+flds[0]+"\t"+flds[1]+"\t"+str_rank;
    int bucket = base::CityHash64(key.c_str(), key.length()) % task->GetReducerNum();
    task->OutputWithReducerID(output_key, value, bucket);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::string output_key;
  while (task->NextKey(&key)) {
    size_t tab_pos = key.find("\t");
    output_key = key.substr(0, tab_pos);
    while (task->NextValue(&value)) {
      task->Output(output_key, value);
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "combine search_log and pv_log show and clk round 2");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false) << "unrecognized app";
  }
  return 0;
}
