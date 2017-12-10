#pragma once
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"

#include "log_analysis/common/log_common.h"

class CombineSearchPV {
 public:
  CombineSearchPV() {
    last_line_type_ = 0;
    last_sid_ = "";
    query_ = "";
    search_logs_.clear();
    timestamp_ = 0;
    start_match = false;
    pv_match = false;
  }
  void ClearStat() {
    search_logs_.clear();
  }
  void AddQuery(const std::string &key, const std::string &value);
  void AddSearchLog(const std::string &key, const std::string &value);
  void Output(base::mapreduce::ReduceTask *task);
  void AddKeyValue(const std::string &key, const std::string &value, base::mapreduce::ReduceTask *task);
 private:
  int last_line_type_;  // 0 for search log and 1 for pv log
  std::string last_sid_;
  std::string query_;
  std::vector<std::string> search_logs_;
  uint64_t timestamp_;
  bool start_match;
  bool pv_match;
  std::string pv_timestamp_;
};
