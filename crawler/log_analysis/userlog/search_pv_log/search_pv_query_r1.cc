// 通过 url 对应的 query 出现次数，投票选出 search log 对应的 query
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(search_log_path, "", "hdfs path of search_log");
DEFINE_string(url_query_path,  "", "hdfs path of url query");


void MapperSearchLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    if (flds.size() < 6) {
      task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
      continue;
    }
    task->Output(flds[5], "S\t"+key);
  }
}

void MapperLastUrlQueryProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string last_key = "";
  int64_t last_key_output = 0;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    last_key_output++;
    if ((last_key == key) && (last_key_output >= 100)) {
        continue;
    } else if (last_key != key) {
      last_key = key;
      last_key_output = 0;
    }
    flds.clear();
    base::SplitString(value, "\t", &flds);
    task->Output(key, "Q\t"+flds[0]);
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  int input_type;
  if (hdfs_path.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_url_query_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << hdfs_path;
  }
  if (input_type == 0) {
    MapperSearchLogProcess(task);
  } else {
    MapperLastUrlQueryProcess(task);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> flds;
  std::vector<std::string> sids;
  std::set<std::string> queries;
  std::string output_value;
  while (task->NextKey(&key)) {
    queries.clear();
    sids.clear();
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.size() != 2) {
        LOG(ERROR) << "input reducer data error";
        continue;
      }
      if (flds[0] == "S") {
        sids.push_back(flds[1]);
      } else if (flds[0] == "Q") {
        queries.insert(flds[1]);
      }
    }
    int64_t queries_count = queries.size();
    if (queries_count == 0) {
      continue;
    }
    std::set<std::string>::iterator it = queries.begin();
    output_value = *it;
    it++;
    for ( ; it != queries.end(); it++) {
      output_value += "\t"+ *it;
    }
    int64_t sids_count = sids.size();
    for (int64_t i = 0; i < sids_count; i++) {
      task->Output(sids[i], output_value);
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log query match");
  CHECK(!FLAGS_search_log_path.empty() && !FLAGS_url_query_path.empty())
      << "specify search_log path and url_query path";
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
  return 0;
}

