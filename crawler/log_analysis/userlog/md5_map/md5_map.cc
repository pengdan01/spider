#include <vector>

#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/base64.h"
#include "web/url/url.h"
#include "nlp/common/nlp_util.h"

DEFINE_string(new_log_path, "", "new log files");
DEFINE_string(history_path,  "", "history log");

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (file_name.find(FLAGS_new_log_path) != std::string::npos) {
    input_type = 0;
  } else if (file_name.find(FLAGS_history_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << file_name;
  }

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (input_type == 0) {
      if (flds.size() < 7u) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      const std::string& md5 = flds[4];
      const std::string& base64 = flds[6];
      if (md5.empty() || base64.empty()) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      std::string url;
      if (!log_analysis::Base64ToClickUrl(base64, &url)) {
        task->ReportAbnormalData(log_analysis::kInvalidURL, md5, base64);
        continue;
      }
      
      std::string time_stamp = "0";
      if (flds.size() > 7) {
        if (!log_analysis::ConvertTimeFromSecToFormat(flds[7], "%Y%m%d%H%M%S", &time_stamp)) {
          time_stamp = "0";
        }
      }
      // base64 -> md5, time_stamp
      task->Output(base64, md5 + "\t" + time_stamp + "\tUPLOAD");
    } else {  // history log
      task->Output(key, value);
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
    std::vector<std::string> history_value;
    std::map<std::string, std::string> md5_map;

    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.back() == "UPLOAD") {
        CHECK_EQ(flds.size(), 3u);
        const std::string& md5 = flds[0];
        std::string time = flds[1];
        auto it = md5_map.find(md5);
        if (it != md5_map.end() && time < it->second) {
          time = it->second;
        }
        md5_map[md5] = time;
      } else {
        history_value.push_back(value);
      }
    }

    CHECK_LE(history_value.size(), 1u);

    if (history_value.empty() && !md5_map.empty()) {
      std::string combine;
      for (auto it = md5_map.begin(); it != md5_map.end(); ++it) {
        if (!combine.empty()) {
          combine += "\t";
        }
        combine = combine + it->first + ":" + it->second;
      }
      task->Output(key, combine);
    } else if (!history_value.empty() && md5_map.empty()) {
      task->Output(key, history_value.front());
    } else {
      flds.clear();
      base::SplitString(history_value.front(), "\t", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], ":", &sub);
        CHECK_EQ(sub.size(), 2u) << flds[i];
        auto it = md5_map.find(sub[0]);
        if (it == md5_map.end()) {
          md5_map[sub[0]] = sub[1];
        } else {
          if (it->second < sub[1]) {
            md5_map[sub[0]] = sub[1];
          }
        }
      }
      std::string combine;
      for (auto it = md5_map.begin(); it != md5_map.end(); ++it) {
        if (!combine.empty()) {
          combine += "\t";
        }
        combine = combine + it->first + ":" + it->second;
      }
      task->Output(key, combine);
      LOG_IF(INFO, md5_map.size() > 1) << "multi md5 detected";
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "md5 map");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
