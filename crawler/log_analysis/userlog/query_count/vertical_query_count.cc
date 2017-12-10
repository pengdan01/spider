#include <vector>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/city.h"
#include "base/encoding/line_escape.h"
#include "nlp/common/nlp_util.h"

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  std::string query;
  std::string site;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    std::string query;
    std::string dedup_id;
    if (flds.size() > 3) {
      dedup_id = flds[0];
      const std::string& url = flds[2];

      if (!url.empty()) {
        if (log_analysis::IsVerticalSearch(url, &query, &site)) {
          task->Output(base::LineEscapeString(query+"\t"+site), dedup_id);
        }
        if (log_analysis::IsSiteInternalSearch(url, &query, &site)) {
          task->Output(base::LineEscapeString(query+"\t"+site), dedup_id);
        }
      }
    }
  }

  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    flds.clear();
    std::string tmp;
    CHECK(base::LineUnescape(key, &tmp));
    base::SplitString(tmp, "\t", &flds);
    CHECK_EQ(flds.size(), 2u);
    key = flds[0];
    std::string site = flds[1];
    std::string value;
    std::unordered_set<std::string> dedup;
    while (task->NextValue(&value)) {
      dedup.insert(value);
    }
    if (dedup.size() > 1) {
      task->OutputWithFilePrefix(key, base::Int64ToString(dedup.size()), site);
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "query count");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
