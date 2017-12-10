#include <vector>
#include <map>
#include <unordered_set>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "base/hash_function/md5.h"
#include "base/encoding/line_escape.h"
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
  std::vector<std::string> subs;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    std::string sid_literal;
    CHECK(base::LineUnescape(flds[0], &sid_literal));
    subs.clear();
    base::SplitString(sid_literal, "\t", &subs);
    CHECK_EQ(subs.size(), 5u) << sid_literal;
    std::string mid = subs[0];
    std::string se_type = subs[1];
    std::string query_md5 = subs[2];
    std::string time_stamp = subs[3];
    std::string rand_string = subs[4];

    int32 reduce_id = base::CityHash64(mid.c_str(), mid.size()) % reducer_count;
    // mid + timestamp + query_md5 + rand_string as compound key
    std::string compound_key = mid + "\t" +
                               time_stamp + "\t" +
                               rand_string + "\t" +
                               query_md5 + "\t" +
                               se_type;

    const std::string& type = flds.back();
    if (flds.size() == 3 && type == "Query") {
      task->OutputWithReducerID(compound_key, flds[1]+"\tQuery", reduce_id);
    } else if (flds.size() == 4 && type == "Url") {
      // rank, url
      task->OutputWithReducerID(compound_key, log_analysis::JoinFields(flds, "\t", "123"), reduce_id);
    } else {
      CHECK(false) << line;
    }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    std::string url;
    std::vector<std::string> queries;
    std::map<std::string, std::string> dedup_url;

    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.back() == "Url" && flds.size() == 3) {
        dedup_url[flds[0]] = flds[1];
      } else if (flds.back() == "Query" && flds.size() == 2) {
        queries.push_back(flds[0]);
      } else {
        CHECK(false) << base::StringPrintf("invalid line: [%s], field [%d]",
                                           line.c_str(), (int)flds.size());
      }
    }
    // CHECK_EQ(queries.size(), 1u) << "query num not 1, key:" << key;
    CHECK_LE(queries.size(), 1u) << "query > 1, key:" << key;

    flds.clear();
    // mid, time, rand_str, query_md5, se_typ, sid
    base::SplitString(key, "\t", &flds);
    CHECK_EQ(flds.size(), 5u);
    std::string query = (queries.size() > 0) ? queries.front() : "";
    const std::string& mid = flds[0];
    const std::string& time_stamp = flds[1];
    const std::string& rand_str = flds[2];
    const std::string& se_type = flds[4];
    std::string sid = MD5String(mid + time_stamp + rand_str + query + se_type);

    for (auto it = dedup_url.begin(); it != dedup_url.end(); ++it) {
      std::string rank = base::IntToString(base::ParseIntOrDie(it->first));
      const std::string& url = it->second;
      // sid, mid, time, query, se_name, rank, url
      task->Output(sid,
                   mid + "\t" +
                   time_stamp + "\t" +
                   query + "\t" +
                   se_type + "\t" +
                   rank + "\t" +
                   url);
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log r3");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
