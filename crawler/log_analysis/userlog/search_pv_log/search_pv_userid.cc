// 通过用户 id 拼接 search log query
#include "search_pv_userid.h"

#include <stdlib.h>
#include <time.h>
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(search_log_path, "", "hdfs path of search_log");
DEFINE_string(pv_log_path,  "", "hdfs path of pv log");

void MapperSearchLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_value;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    int bucket = base::CityHash64(flds[0].c_str(), flds[0].length()) % task->GetReducerNum();
    base::LineEscape(key+"\t"+value, &output_value);
    task->OutputWithReducerID(flds[0]+"\t"+flds[1]+"\t"+key+"A", output_value, bucket);
  }
}

void MapperPVLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_value;
  std::string query;
  std::string search_engine;
  std::string charset;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    int bucket = base::CityHash64(key.c_str(), key.length()) % task->GetReducerNum();
    if (log_analysis::ParseSearchQuery(flds[1], &query, &search_engine, &charset)) {
      task->OutputWithReducerID(key+"\t"+flds[0]+"B", query, bucket);
    }
  }
}

void CombineSearchPV::AddQuery(const std::string &key, const std::string &value) {
  std::vector<std::string> flds;
  std::string pv_data;
  base::SplitString(value, "\t", &flds);
  query_ = flds[0];

  flds.clear();
  base::SplitString(key, "\t", &flds);
  pv_timestamp_ = flds[1];
  pv_timestamp_ = pv_timestamp_.substr(0, pv_timestamp_.length()-1);
}

void CombineSearchPV::AddSearchLog(const std::string &key, const std::string &value) {
  search_logs_.push_back(value);
}

void CombineSearchPV::Output(base::mapreduce::ReduceTask *task) {
  size_t search_log_size = search_logs_.size();
  std::string unescape_data;
  std::vector<std::string> flds;
  for (size_t i = 0; i < search_log_size; i++) {
    CHECK(base::LineUnescape(search_logs_[i], &unescape_data));
    flds.clear();
    base::SplitString(unescape_data, "\t", &flds);
    if (flds.size() != 7) {
      task->ReportAbnormalData(log_analysis::kMapInputError, "", search_logs_[i]);
      continue;
    }
    task->Output(flds[0], flds[1]+"\t"+flds[2]+"\t"+query_+"\t"+flds[4]
                 +"\t"+flds[5]+"\t"+flds[6]+"\t"+pv_timestamp_);
  }
}

void CombineSearchPV::AddKeyValue(const std::string &key,
                                  const std::string &value,
                                  base::mapreduce::ReduceTask *task) {
  int ret = 0;
  std::string unescape_data;
  std::string this_sid;
  std::vector<std::string> flds;
  std::string ts;
  struct tm tmp_time;
  // check which log
  if (key[key.length()-1] == 'B') {
    // pv log
    size_t tab_pos = key.find("\t");
    ts = key.substr(tab_pos+1, key.length()-tab_pos-2);
    strptime(ts.c_str(), "%Y%m%d%H%M%S", &tmp_time);
    uint64_t timestamp = mktime(&tmp_time);
    ret = 0;
    pv_match = false;
    if (last_line_type_ == 1) {
      this->ClearStat();
      this->AddQuery(key, value);
      timestamp_ = timestamp;
      last_line_type_ = 1;
      start_match = false;
      ret = 0;
    } else if ((last_line_type_ == 0) && (start_match == 0)) {
      if (timestamp - timestamp_ < 5) {
        this->AddQuery(key, value);
        this->Output(task);
        last_line_type_ = 1;
        timestamp_ = 0;
        start_match = false;
        pv_match = true;
        ret = 0;
      } else {
        this->ClearStat();
        this->AddQuery(key, value);
        timestamp_ = timestamp;
        last_line_type_ = 1;
        start_match = false;
        ret = 0;
      }
    } else {
      this->ClearStat();
      this->AddQuery(key, value);
      timestamp_ = timestamp;
      last_line_type_ = 1;
      start_match = false;
      ret = 0;
    }
  } else if (key[key.length()-1] == 'A') {
    CHECK(base::LineUnescape(value, &unescape_data));
    flds.clear();
    base::SplitString(unescape_data, "\t", &flds);
    this_sid = flds[0];
    size_t tab_pos = key.find("\t");
    size_t tab_pos2 = key.find("\t", tab_pos+1);
    CHECK(tab_pos != std::string::npos);
    CHECK(tab_pos2 != std::string::npos);
    ts = key.substr(tab_pos+1, tab_pos2-tab_pos-1);
    strptime(ts.c_str(), "%Y%m%d%H%M%S", &tmp_time);
    uint64_t timestamp = mktime(&tmp_time);
    if (last_line_type_ == 0) {
      if (last_sid_ != this_sid) {
        this->ClearStat();
        this->AddSearchLog(key, value);
        last_sid_ = this_sid;
        last_line_type_ = 0;
        timestamp_ =  timestamp;
        start_match = false;
      } else {
        if (start_match == true) {
          task->Output(flds[0], flds[1]+"\t"+flds[2]+"\t"+query_+"\t"
                       +flds[4]+"\t"+flds[5]+"\t"+flds[6]+"\t"+pv_timestamp_);
          last_sid_ = this_sid;
          last_line_type_ = 0;
          timestamp_ =  timestamp;
          start_match = true;
        } else {
          this->AddSearchLog(key, value);
          last_sid_ = this_sid;
          last_line_type_ = 0;
          timestamp_ =  timestamp;
          start_match = false;
        }
      }
    } else if (last_line_type_ == 1) {
      if ((timestamp - timestamp_ < 5) && (pv_match == false)) {
        task->Output(flds[0], flds[1]+"\t"+flds[2]+"\t"+query_+"\t"
                     +flds[4]+"\t"+flds[5]+"\t"+flds[6]+"\t"+pv_timestamp_);
        last_line_type_ = 0;
        last_sid_ = this_sid;
        start_match = true;
        timestamp_ =  timestamp;
      } else {
        this->ClearStat();
        this->AddSearchLog(key, value);
        last_sid_ = this_sid;
        last_line_type_ = 0;
        timestamp_ =  timestamp;
        start_match = false;
      }
    }
  } else {
    LOG(ERROR) << "unknow type";
    ret = -1;
  }
}

void Reducer() {
  CombineSearchPV *combine_sv = new CombineSearchPV;
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      combine_sv->AddKeyValue(key, value, task);
    }
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  int input_type;
  if (hdfs_path.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_pv_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << hdfs_path;
  }
  if (input_type == 0) {
    MapperSearchLogProcess(task);
  } else {
    MapperPVLogProcess(task);
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "combine pv log and search log by user id");
  CHECK(!FLAGS_search_log_path.empty() && !FLAGS_pv_log_path.empty())
      << "specify search_log path and pv_log path";
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false) << "unrecognized app";
  }
}
