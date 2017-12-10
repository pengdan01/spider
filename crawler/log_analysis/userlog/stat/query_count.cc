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
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

DEFINE_string(pv_log_path, "", "hdfs path of pv log");
DEFINE_string(search_log_path, "", "hdfs path of search log");

void mapper_output(base::mapreduce::MapTask *task, const std::string& query,
                   const std::string& site, const std::string& dedup_id) {
  static int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  if (site.empty()) {
    task->Output(query, dedup_id);
  } else {
    int32 reduce_id = base::CityHash64(query.c_str(), query.size()) % reducer_count;
    std::string compound_key = query + "\t" + site;
    task->OutputWithReducerID(compound_key, dedup_id, reduce_id);
  }
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (!FLAGS_pv_log_path.empty() && file_name.find(FLAGS_pv_log_path) != std::string::npos) {
    input_type = 0;
  } else if (!FLAGS_search_log_path.empty() && file_name.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << file_name;
  }

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  std::string query;
  std::string site;
  bool suc;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    std::string query;
    std::string dedup_id;
    if (input_type == 0) {  // pv log
      if (flds.size() > 5) {
        dedup_id = flds[0];
        const std::string& url = flds[2];
        // const std::string& ref_url = flds[3];

        if (!url.empty()) {
          suc = log_analysis::IsGeneralSearch(url, &query, &site);
          if (suc) {
            site.clear();
            mapper_output(task, query, site, dedup_id);
          }

          // suc = log_analysis::ParseVerticalSiteQuery(url, &query, &site);
          // if (suc) {
          //   mapper_output(task, query, site, dedup_id);
          // }
        }

        // if (!ref_url.empty()) {
        //   suc = log_analysis::ParseSearchQuery(ref_url, &query, &site, &charset);
        //   if (suc) {
        //     mapper_output(task, query, site, dedup_id);
        //   }

        //   suc = log_analysis::ParseVerticalSiteQuery(ref_url, &query, &site);
        //   if (suc) {
        //     mapper_output(task, query, site, dedup_id);
        //   }
        // }
      }
    } else {  // search log
      if (flds.size() > 4) {
        query = flds[3];
        dedup_id = flds[0];
        if (!query.empty() && !dedup_id.empty()) {
          mapper_output(task, query, "", dedup_id);
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
    std::string value;
    std::unordered_set<std::string> dedup;
    while (task->NextValue(&value)) {
      dedup.insert(value);
    }
    if (dedup.size() > 0) {
      task->Output(key, base::IntToString(dedup.size()));
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
