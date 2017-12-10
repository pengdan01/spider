#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>

#include "log_analysis/common/error_code.h"
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
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);

    // [ mid, time_stamp, url, referer_url, '', '', attr, enter_type, duration]
    int32 reduce_id = base::CityHash64(flds[0].c_str(), flds[0].size()) % reducer_count;
    std::string compound_key = flds[0] + "\t" + flds[1];  // mid + time_stamp as compound key
    if (flds.size() == 9) {
      task->OutputWithReducerID(compound_key, log_analysis::JoinFields(flds, "\t", "2345678"), reduce_id);
    } else if (flds.size() == 10 && flds.back() == "still_md5") {
      task->OutputWithReducerID(compound_key, log_analysis::JoinFields(flds, "\t", "23456789"), reduce_id);
    } else if (flds.size() == 4) {
      task->OutputWithReducerID(compound_key, log_analysis::JoinFields(flds, "\t", "23"), reduce_id);
    } else {
      LOG(FATAL) << base::StringPrintf("invalid line: [%s], field [%d]",
                                       line.c_str(), (int)flds.size());
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
    std::map<std::string, std::string> md5_map;
    std::set<std::string> dedup_final;
    std::set<std::string> dedup_temp;

    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.size() == 2) {
        md5_map[flds[0]] = flds[1];
      } else if (flds.size() == 8 && flds.back() == "still_md5") {
        dedup_temp.insert(log_analysis::JoinFields(flds, "\t",
                                               "0123456"));
      } else if (flds.size() == 7) {
        dedup_final.insert(value);
      } else {
        CHECK(false);
      }
    }
    for (auto it = dedup_temp.begin(); it != dedup_temp.end(); ++it) {
      flds.clear();
      base::SplitString(*it, "\t", &flds);
      CHECK_EQ(flds.size(), 7u);
      std::string& dest = flds[0];
      std::string& ref =  flds[1];
      auto jt = md5_map.find(dest);
      if (jt != md5_map.end()) {
        dest = jt->second;
      } else {
        task->ReportAbnormalData(log_analysis::kInvalidURL, dest, "");
        continue;
      }
      jt = md5_map.find(ref);
      if (jt != md5_map.end()) {
        ref = jt->second;
      } else {
        ref.clear();
      }
      dedup_final.insert(base::JoinStrings(flds, "\t"));
    }
    for (auto it = dedup_final.begin(); it != dedup_final.end(); ++it) {
      // [ mid, time_stamp, url, referer_url, '', '', attr, enter_type, duration]
      // parse query from referer_url
      flds.clear();
      base::SplitString(*it, "\t", &flds);
      LOG_IF(FATAL, flds.size() != 7);
      // const std::string& ref_url = flds[1];
      // std::string ref_query;
      // std::string search_engine;
      // std::string charset;
      // bool ret = log_analysis::ParseSearchQuery(ref_url, &ref_query, &search_engine, &charset);
      // if (ret) {
      //   flds[2] = nlp::util::NormalizeLine(ref_query);
      //   if (flds[2].empty()) {
      //     task->ReportAbnormalData(log_analysis::kInvalidQuery, ref_query, "");
      //     continue;
      //   }
      // }
      // flds.insert(flds.begin() + 3, search_engine);
      task->Output(key, log_analysis::JoinFields(flds, "\t", "01456"));
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "pv log r2");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
