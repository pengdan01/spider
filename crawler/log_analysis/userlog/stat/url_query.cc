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

// DEFINE_string(history_data_path, "", "hdfs path of history url-query pv data");
// DEFINE_string(pv_log_path,  "", "hdfs path of pv log");

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  // base::mapreduce::InputSplit split = task->GetInputSplit();

  // int input_type = 0;
  // const std::string& hdfs_path = split.hdfs_path;
  // if (hdfs_path.find(FLAGS_history_data_path) != std::string::npos) {
  //   // history data
  //   input_type = 0;
  // } else if (hdfs_path.find(FLAGS_pv_log_path) != std::string::npos) {
  //   // pv_log
  //   input_type = 1;
  // } else {
  //   LOG(FATAL) << "unexpected input file: " << hdfs_path;
  // }

  std::string key;
  std::string value;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    if (flds.size() == 3) {  // history data
      task->Output(key + "\t" + flds[0] + "\t" + flds[1], flds[2]);
    } else {  // pv_log
      CHECK_EQ(flds.size(), 6u) << value;

      const std::string& dest_url = flds[1];
      const std::string& ref_url = flds[2];
      std::string query, se;
      if (log_analysis::IsGeneralSearchClick(dest_url, ref_url, &query, &se)) {
        std::string target_url;
        if (se == log_analysis::kGoogle) {
          // 如果是 google 的结果, 需要做跳转连接识别
          target_url = log_analysis::GoogleTargetUrl(dest_url);
        } else {
          target_url = dest_url;
        }
        task->Output(target_url + "\t" + se + "\t" + query, "1");
      }
    }
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key,value;
  while (task->NextKey(&key)) {
    int64 count = 0;
    while (task->NextValue(&value)) {
      count += base::ParseInt64OrDie(value);
    }
    task->Output(key, base::Int64ToString(count));
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

