// 通过 pv log 和 searchlog 拼接 show 和 clk
// 如果缺少 query 字段, 就跳过该条日志
#include <stdlib.h>
#include <time.h>
#include <map>
#include <list>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_number_conversions.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(search_log_path, "", "hdfs path of search_log");
DEFINE_string(pv_log_path,  "", "hdfs path of pv log");

typedef std::list<std::pair<std::string, int64_t>> SearchLogClick;
static const int64_t KPvSearchLogTimeGapThreshold = 60 * 10;
void MapperSearchLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_key;
  std::string output_value;
  int rank;
  while (task->NextKeyValue(&key, &value)) {
    // pack search log data
    base::LineEscape(key+"\t"+value, &output_value);
    // value: uid, timestamp, query, se_name, rank, res_url
    flds.clear();
    base::SplitString(value, "\t", &flds);
    CHECK_EQ(flds.size(), 6u);
    // 过滤没有 query 的日志
    if (flds[2] == "") {
      continue;
    }
    base::StringToInt(flds[4], &rank);
    // if (rank > 10) {
    //   // filter ads
    //   continue;
    // }
    const std::string& uid = flds[0];
    const std::string& query = flds[2];
    const std::string& search_engine =flds[3];
    const std::string& target_url = flds[5];

    std::string output_url = target_url;
    if (search_engine == log_analysis::kGoogle) {
      if (target_url.find("/url?") != std::string::npos) {
        if (!log_analysis::ParseGoogleTargetUrl(target_url, &output_url)) {
          output_url = target_url;
        }
      }
    }
    // key: uid, res_url, query, search_engine, "A"
    task->Output(uid + "\t" + output_url + "\t" + query + "\t" + search_engine,
                 output_value + "\tA");
  }
}

void MapperPVLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_key;
  std::string query_ref;
  std::string search_engine_ref;
  std::string target_url;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    // key: uid,
    // value: timestamp, url, ref_url, url_attr, entrance_id, duration
    const std::string& url = flds[1];
    const std::string& ref_url = flds[2];
    if (log_analysis::IsGeneralSearch(ref_url, &query_ref, &search_engine_ref)) {  // ref
    //if (log_analysis::IsGeneralSearchClick(url, ref_url, &query_ref, &search_engine_ref)) {  // ref
      // if (!log_analysis::ParseSearchQuery(url, &query, &search_engine, &charset)) {  // dest
        if (search_engine_ref == log_analysis::kGoogle) {
          if (!log_analysis::ParseGoogleTargetUrl(url, &target_url)) {
            if (url.find("/url?") != std::string::npos) {
              task->ReportAbnormalData("Google Url Parse Failed", key, url);
            }
            target_url = url;
          }
        } else {
          target_url = url;
        }
        const std::string& pv_log_timestamp = flds[0];
        // key: uid, target_url, refer query, refer search_engine  pv_log_timestamp, "B"
        task->Output(key + "\t" + target_url + "\t" + query_ref + "\t" + search_engine_ref,
                     pv_log_timestamp + "\tB");
      // }
    }
  }
}

SearchLogClick::iterator FindClickSearchLog(int64_t pv_log_timestamp,
                                            SearchLogClick *search_log_click) {
  int64_t min_time_gap = pv_log_timestamp;
  auto it = search_log_click->begin();
  auto min_it = it;
  for ( ; it != search_log_click->end(); ++it) {
    int64_t gap = labs(it->second - pv_log_timestamp);
    if (gap < min_time_gap) {
      min_time_gap = gap;
      min_it = it;
    }
  }
  return min_it;
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::vector<std::string> search_log_flds;
  std::vector<int64_t> click_time;
  SearchLogClick search_log_click; // search_log, search_log_timestamp
  std::string unescape_search_log;
  int64_t search_log_timestamp;
  int64_t pv_log_timestamp;
  while (task->NextKey(&key)) {
    search_log_click.clear();
    click_time.clear();
    while (task->NextValue(&value)) {
      std::string line = key + "\t" + value;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      const std::string& type = flds.back();
      if (type == "A") {  // search log
        CHECK_EQ(flds.size(), 6u);
        const std::string& search_log = flds[4];
        CHECK(base::LineUnescape(search_log, &unescape_search_log));
        // 获得 search_log 时间戳
        search_log_flds.clear();
        base::SplitString(unescape_search_log, "\t", &search_log_flds);
        CHECK(log_analysis::ConvertTimeFromFormatToSec(search_log_flds[2],
                                                       "%Y%m%d%H%M%S",
                                                       &search_log_timestamp));
        search_log_click.push_back(make_pair(unescape_search_log, search_log_timestamp));
      } else if (type == "B") {  // pv　log
        CHECK_EQ(flds.size(), 6u);
        CHECK(log_analysis::ConvertTimeFromFormatToSec(flds[4], "%Y%m%d%H%M%S", &pv_log_timestamp));
        click_time.push_back(pv_log_timestamp);
      } else {
        CHECK(false) << line;
      }
    }
    if (search_log_click.size() == 0UL) {
      continue;
    }
    // 对每一个 click, 找到其时间戳与 search log 时间戳最相近的 search_log
    for (size_t i = 0; i < click_time.size(); ++i) {
      auto it = FindClickSearchLog(pv_log_timestamp, &search_log_click);
      if (it == search_log_click.end()) {
        // 表明 search_log_click 已经为空了，注意如果有两次点击，我们只取一次，也就是 show 不能大于 click
        CHECK_EQ(search_log_click.size(), 0UL);
        break;
      }
      // 检查时间戳间隔，如果大于一定阈值，就不算作点击
      if (labs(it->second - pv_log_timestamp) > KPvSearchLogTimeGapThreshold) {
        LOG(INFO) << "drop click, timegap: " << labs(it->second - pv_log_timestamp) 
                  << "larger than " << KPvSearchLogTimeGapThreshold;
        continue;
      }
      // 输出这个值,其中 click 为 1
      size_t tab_pos = it->first.find("\t");
      CHECK_NE(tab_pos, std::string::npos);
      task->Output(it->first.substr(0, tab_pos),
                   it->first.substr(tab_pos+1, it->first.size() - tab_pos - 1) + "\t1\t1");
      // 删除 it
      search_log_click.erase(it);
    }

    auto it = search_log_click.begin();
    for ( ; it != search_log_click.end(); ++it) {
      size_t tab_pos = it->first.find("\t");
      CHECK_NE(tab_pos, std::string::npos);
      task->Output(it->first.substr(0, tab_pos),
                   it->first.substr(tab_pos+1, it->first.size() - tab_pos - 1) + "\t1\t0");
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
  base::mapreduce::InitMapReduce(&argc, &argv, "combine search_log and pv_log show clk");
  CHECK(!FLAGS_search_log_path.empty() && !FLAGS_pv_log_path.empty())
      << "specify search_log path and pv_log path";
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
