// 找出一个 url 命中的 query 以及命中次数
#include <set>
#include <map>
#include <utility>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/hash_function/term.h"
#include "base/hash_function/url.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_printf.h"

#include "log_analysis/common/log_common.h"
#include "query/parser2/query_parser.h"
#include "query/dicts/query_dicts.h"

#include "web/url_norm/url_norm.h"

void proc(const std::string& query, const std::vector<std::string>& records,
          queries::QueryParser* parser, web::UrlNorm* url_norm,
          base::mapreduce::MapTask *task) {
  std::string norm_query;
  if (!queries::NormalizeQueryS(query, parser, &norm_query)) {
    return;
  }

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  uint64 query_sign = base::CalcTermSign(norm_query.c_str(), norm_query.length());
  int reduce_id = query_sign % reducer_count;
  std::vector<std::string> flds;

  for (size_t i = 0; i < records.size(); ++i) {
    flds.clear();
    base::SplitString(records[i], "\t", &flds);
    if (flds.size() != 4u) continue;

    // url se rank count
    const std::string& url = flds[0];
    std::string norm_url;
    if (!url_norm->NormalizeClickUrl(url, &norm_url)) {
      continue;
    }
    const std::string& se = flds[1];

    int rank_num = 0;
    if (!base::StringToInt(flds[2], &rank_num)) continue;
    std::string rank = base::StringPrintf("%02d", rank_num);

    std::string key = base::StringPrintf("%s\t%s\t%s", norm_query, se, rank);
    std::string value = base::StringPrintf("%s\t%s", url, count);
    task->OutputWithReducerID(key, value, reduce_id);
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  queries::QueryParser parser;
  queries::QueryDicts  dicts;
  dicts.LoadAllDicts();
  parser.SetDicts(&dicts);
  web::UrlNorm url_norm;

  std::string last_key;
  std::string key;
  std::string value;
  std::vector<std::string> records;
  // query url se rank count
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    if (key != last_key && !last_key.empty()) {
      proc(last_key, records, &parser, &url_norm, task);
      last_key.clear();
      records.clear();
    }
    records.push_back(value);
    last_key = key;
  }
  if (key != last_key && !last_key.empty()) {
    proc(last_key, records, &parser, &url_norm, task);
    last_key.clear();
    records.clear();
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key,value;
  std::vector<std::string> flds;
  std::map<std::string, int64> counts;
  while (task->NextKey(&key)) {
    flds.clear();
    base::SplitString(key, "\t", &flds);
    if (flds.size() != 3) continue;
    std::string query = flds[0];
    int rank = base::ParseInt32OrDie(flds[1]);
    counts.clear();
    while (task->NextValue(&value)) {
      counts[value] += 1;
    }
    std::pair<std::string, int64> max_url;
    max_url.second = 0;
    for (auto it = counts.begin(); it!= counts.end(); ++it) {
      if (it->second > max_url.second) {
        max_url.second = it->second;
        max_url.first = it->first;
      }
    }
    if (max_url.first.empty()) continue;
    task->Output(query,
                 base::IntToString(rank) + "\t"
                 + max_url.first + "\t"
                 + base::Int64ToString(max_url.second));
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url_query_count");
  if (base::mapreduce::IsMapApp()) {
    // CHECK(!FLAGS_history_data_path.empty() || !FLAGS_pv_log_path.empty()) 
    //   << "specify history data path and pv_log path";
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}

