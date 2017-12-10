// 通过 url 对应的 query 出现次数，投票选出 search log 对应的 query
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(last_round_path, "", "hdfs path of last round path");
DEFINE_string(search_log_path,  "", "hdfs path of search log");
DEFINE_double(query_threshold, 0.5, "only output queries that higher than query_threshold");

void MapperSearchLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    task->Output(key, "S\t"+value);
  }
}

void MapperLastRoundProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    task->Output(key, "L\t"+value);
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  int input_type;
  if (hdfs_path.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_last_round_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << hdfs_path;
  }
  if (input_type == 0) {
    MapperSearchLogProcess(task);
  } else {
    MapperLastRoundProcess(task);
  }
}

void FindMaxQueries(std::map<std::string, int> *queries, std::string *max_query, int *max_count) {
  *max_count = 0;
  *max_query = "";
  std::map<std::string, int>::iterator it = queries->begin();
  for (; it != queries->end(); it++) {
    if (it->second > *max_count) {
      *max_count = it->second;
      *max_query = it->first;
    }
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> flds;
  std::map<int, std::string> search_logs;
  std::map<std::string, int> queries;
  while (task->NextKey(&key)) {
    search_logs.clear();
    queries.clear();
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds[0] == "S") {
        int rank = atoi(flds[5].c_str());
        search_logs[rank] = value;
      } else {
        size_t flds_size = flds.size();
        for (size_t i = 1; i < flds_size; i++) {
          queries[flds[i]]++;
        }
      }
    }
    if (queries.size() == 0) {
      continue;
    }
    std::string max_query;
    int max_count;
    FindMaxQueries(&queries, &max_query, &max_count);
    size_t search_log_size = search_logs.size();
    if (FLAGS_query_threshold > static_cast<double>(max_count)/search_log_size) {
      continue;
    }
    std::string output_string;
    std::map<int, std::string>::iterator it = search_logs.begin();
    for (; it != search_logs.end(); it++) {
      flds.clear();
      base::SplitString(it->second, "\t", &flds);
      output_string = flds[1]+"\t";
      output_string += flds[2]+"\t";
      output_string += max_query+"\t";
      output_string += flds[4]+"\t";
      output_string += flds[5]+"\t";
      output_string += flds[6];
      task->Output(key, output_string);
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log query match round 2");
  CHECK(!FLAGS_search_log_path.empty() && !FLAGS_last_round_path.empty())
      << "specify search_log path and pv_log path";
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
