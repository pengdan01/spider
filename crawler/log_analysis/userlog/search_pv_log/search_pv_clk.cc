// 通过 pv log 和已经拼接上 query 的 searchlog 拼接 show 和 clk
#include <stdlib.h>
#include <time.h>
#include <map>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_number_conversions.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(search_log_path, "", "hdfs path of search_log(has pv log timestamp)");
DEFINE_string(pv_log_path,  "", "hdfs path of pv log");

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
    // value: uid, timestamp, query, se_name, rank, res_url, pv_log_timestamp
    flds.clear();
    base::SplitString(value, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    base::StringToInt(flds[4], &rank);
    if (rank > 10) {
      // filter ads
      continue;
    }
    const std::string& uid = flds[0];
    const std::string& query = flds[2];
    const std::string& search_engine =flds[3];
    const std::string& target_url = flds[5];
    const std::string& pv_log_timestamp = flds[6];

    std::string output_url = target_url;
    if (search_engine == log_analysis::kGoogle) {
      if (target_url.find("/url?") != std::string::npos) {
        if (!log_analysis::ParseGoogleTargetUrl(target_url, &output_url)) {
          output_url = target_url;
        }
      }
    }
    // key: uid, res_url, query, search_engine, pv_log_timestamp, "A"
    task->Output(uid + "\t" + output_url + "\t" + query + "\t" + search_engine,
                 pv_log_timestamp + "\t" + output_value + "\tA");
  }
}

void MapperPVLogProcess(base::mapreduce::MapTask *task) {
  std::string key;
  std::string value;
  std::vector<std::string> flds;
  std::string output_key;
  std::string query, query_ref;
  std::string search_engine, search_engine_ref;
  std::string charset, charset_ref;
  std::string target_url;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    flds.clear();
    base::SplitString(value, "\t", &flds);
    // key: uid,
    // value: timestamp, url, ref_url, url_attr, entrance_id, duration
    const std::string& url = flds[1];
    const std::string& ref_url = flds[2];
    if (log_analysis::ParseSearchQuery(ref_url, &query_ref, &search_engine_ref, &charset_ref)) {  // ref
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

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> flds;
  std::string unescape_search_log;
  while (task->NextKey(&key)) {
    std::map<std::string, std::string> time_to_search_log;
    std::set<std::string> click_time;
    while (task->NextValue(&value)) {
      std::string line = key + "\t" + value;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      const std::string& type = flds.back();
      if (type == "A") {  // search log
        CHECK_EQ(flds.size(), 7u);
        const std::string& pv_log_timestamp = flds[4];
        const std::string& search_log = flds[5];
        if (time_to_search_log.find(pv_log_timestamp) != time_to_search_log.end()) {
          // 一次检索可能出现两个 url 完全一样的结果，把 clk 累计到第一次出现的 instane 上
          CHECK(base::LineUnescape(search_log, &unescape_search_log));
          size_t tab_pos = unescape_search_log.find("\t");
          CHECK(tab_pos != std::string::npos);
          task->Output(unescape_search_log.substr(0, tab_pos),
                       unescape_search_log.substr(tab_pos+1, unescape_search_log.length()-tab_pos-1)
                       + "\t1\t0");
        } else {
          time_to_search_log[pv_log_timestamp] = search_log;
        }
      } else if (type == "B") {  // pv　log
        CHECK_EQ(flds.size(), 6u);
        const std::string& pv_log_timestamp = flds[4];
        click_time.insert(pv_log_timestamp);
      } else {
        CHECK(false) << line;
      }
    }
    // for each search log, find its click
    auto search_it = time_to_search_log.begin();
    auto pv_it = click_time.begin();
    std::map<std::string, std::string> click_show_info;
    // auto click_it = click_show_info.begin();
    while (search_it != time_to_search_log.end() &&
           pv_it != click_time.end()) {
      if (search_it->first > *pv_it) {  // pv 在　search 之前
        pv_it++;
      } else if (search_it->first <= *pv_it) {  // search 在　pv 之前
        // 把上一条 search_log 覆盖掉，会丢 show
        auto click_it = click_show_info.find(*pv_it);
        if (click_it != click_show_info.end()) {
          CHECK(base::LineUnescape(click_it->second, &unescape_search_log));
          size_t tab_pos = unescape_search_log.find("\t");
          CHECK(tab_pos != std::string::npos);
          task->Output(unescape_search_log.substr(0, tab_pos),
                       unescape_search_log.substr(tab_pos+1, unescape_search_log.length()-tab_pos-1)
                       + "\t1\t0");
        }
        click_show_info[*pv_it] = search_it->second;
        search_it++;
      }
    }

    while (search_it != time_to_search_log.end()) {  // 点击已经结束了
      // output 1 0
      CHECK(base::LineUnescape(search_it->second, &unescape_search_log));
      size_t tab_pos = unescape_search_log.find("\t");
      CHECK(tab_pos != std::string::npos);
      task->Output(unescape_search_log.substr(0, tab_pos),
                   unescape_search_log.substr(tab_pos+1, unescape_search_log.length()-tab_pos-1)
                   + "\t1\t0");
      search_it++;
    }
    auto clk_it = click_show_info.begin();
    for (; clk_it != click_show_info.end(); ++clk_it) {
      CHECK(base::LineUnescape(clk_it->second, &unescape_search_log));
      size_t tab_pos = unescape_search_log.find("\t");
      CHECK(tab_pos != std::string::npos);
      task->Output(unescape_search_log.substr(0, tab_pos),
                   unescape_search_log.substr(tab_pos+1, unescape_search_log.length()-tab_pos-1)
                   + "\t1\t1");
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
