// 找出一个 url 命中的 query 以及命中次数
#include <set>
#include <map>
#include <utility>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "log_analysis/common/log_common.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_number_conversions.h"

DEFINE_string(history_data_path, "", "hdfs path of history url-query pv data");
DEFINE_string(pv_log_path,  "", "hdfs path of pv log");

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();

  int input_type = 0;
  const std::string& hdfs_path = split.hdfs_path;
  if (hdfs_path.find(FLAGS_history_data_path) != std::string::npos) {
    // history data
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_pv_log_path) != std::string::npos) {
    // pv_log
    input_type = 1;
  } else {
    LOG(FATAL) << "unexpected input file: " << hdfs_path;
  }

  std::string key;
  std::string value;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    if (input_type == 0) {  // history data
      task->Output(key, value);
    } else {  // pv_log
      flds.clear();
      base::SplitString(value, "\t", &flds);
      CHECK_GT(flds.size(), 3u) << value;

      std::string query;
      std::string search_engine;
      std::string charset;
      std::string query_ref;
      std::string search_engine_ref;
      std::string charset_ref;
      const std::string& url = flds[1];
      const std::string& ref_url = flds[2];

      // ref_url is a search engine, dest url is not
      if (log_analysis::ParseSearchQuery(ref_url, &query_ref, &search_engine_ref, &charset_ref) &&
          !log_analysis::ParseSearchQuery(url, &query, &search_engine, &charset)) {
        if (search_engine_ref == log_analysis::kBaidu ||
            search_engine_ref == log_analysis::kGoogle ||
            search_engine_ref == log_analysis::kBing ||
            search_engine_ref == log_analysis::kSogou ||
            search_engine_ref == log_analysis::kYahoo) {
          task->Output(url, query_ref + "\t1");
        }
      }
    }
  }
}

bool sort_cmp(const std::pair<std::string, int>& i, const std::pair<std::string, int>& j) {
  return i.second > j.second;
}

void MapSortAndOutput(const std::map<std::string, int> &queries,
                     const std::string &key,
                     base::mapreduce::ReduceTask *task) {
  std::vector<std::pair<std::string, int>> temp_vector;
  temp_vector.reserve(queries.size());
  for (auto it = queries.begin(); it != queries.end(); ++it) {
    temp_vector.push_back(make_pair(it->first, it->second));
  }
  sort(temp_vector.begin(), temp_vector.end(), sort_cmp);
  for (auto it = temp_vector.begin(); it != temp_vector.end(); ++it) {
    task->Output(key, it->first + "\t" + base::IntToString(it->second));
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key,value;
  std::map<std::string, int> queries;
  std::string output_escape_value;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    queries.clear();
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      queries[flds[0]] += base::ParseInt64OrDie(flds[1]);
    }
    int64_t queries_count = queries.size();
    if (queries_count == 0) {
      continue;
    }
    MapSortAndOutput(queries, key, task);
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url_query_count");
  CHECK(!FLAGS_history_data_path.empty() || !FLAGS_pv_log_path.empty()) 
      << "specify history data path and pv_log path";
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}

