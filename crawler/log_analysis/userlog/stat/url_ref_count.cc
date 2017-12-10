#include <vector>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

DEFINE_string(pv_log_path, "", "hdfs path of pv log");
DEFINE_string(search_log_path, "", "hdfs path of search log");

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    std::string& url = flds[2];
    std::string& ref_url = flds[3];
    if (!url.empty()) {
      task->Output(url + "\t" + ref_url, "");
    }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    int count = 0;
    while (task->NextValue(&value)) {
      count++;
    }
    flds.clear();
    base::SplitString(key, "\t", &flds);
    CHECK_EQ(flds.size(), 2u) << "invalid line: " << key;
    if (count > 0) {
      task->Output(flds[0], flds[1]);
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url ref count");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
