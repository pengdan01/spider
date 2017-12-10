#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/time/timestamp.h"
#include "base/hash_function/md5.h"
#include "base/hash_function/city.h"
#include "base/encoding/line_escape.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"


DEFINE_string(pv_log_path, "", "pv log path");
DEFINE_string(search_log_path,  "", "hdfs path of pv log");
DEFINE_int32(elapse, 5, "time elapse between search and pv");
DEFINE_int32(click_elapse, 120, "time elapse between search and pv");
DEFINE_int32(query_elapse, 120, "time elapse between search and pv");
DEFINE_int32(type, 3, "");
DEFINE_string(key, "", "");

void ProcessSearchLog(const std::vector<std::string>& buffer,
                      base::mapreduce::MapTask *task) {
  if (buffer.size() == 0) return;

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  std::vector<std::string> flds;
  const std::string& line = buffer[0];
  flds.clear();
  base::SplitString(line, "\t", &flds);
  CHECK_EQ(flds.size(), 7u);
  std::string sid = flds[0];
  std::string mid = flds[1];
  std::string timestamp = flds[2];
  std::string query = flds[3];
  std::string sename = flds[4];

  std::vector<std::string> rank_url;
  for (int i = 0; i < (int)buffer.size(); ++i) {
    const std::string& line = buffer[i];
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    CHECK_EQ(flds[0], sid);
    const std::string& rank = flds[5];
    const std::string& url = flds[6];

    rank_url.push_back(rank + "\t" + url);
  }
  
  int32 reduce_id = base::CityHash64(mid.c_str(), mid.size()) % reducer_count;
  std::string compound_key = mid + "\t" + sename;
  task->OutputWithReducerID(compound_key,
                            timestamp + "\t" + query + "\t" + sid + "\t" +
                            base::LineEscapeString(base::JoinStrings(rank_url, "\t")),
                            reduce_id);
}

struct Info {
  std::string time_stamp;  // 搜索的时间
  std::string query;
  std::vector<std::string> clicked_urls; // 点击的 url
};

void ProcessPvLog(const std::vector<std::string>& buffer,
                  base::mapreduce::MapTask *task) {
  if (buffer.size() == 0) return;

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  // 以搜索引擎+搜索页面为key, 收集点击
  std::multimap<std::string, Info> collection;

  std::vector<std::string> flds;
  std::string common_mid;
  for (int i = 0; i < (int)buffer.size(); ++i) {
    const std::string& line = buffer[i];
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    const std::string& mid = flds[0];
    const std::string& timestamp = flds[1];
    const std::string& dest_url = flds[2];
    const std::string& ref_url = flds[3];

    if (common_mid.empty()) {
      common_mid = mid;
    }

    std::string query, se_name;
    if (log_analysis::IsGeneralSearch(dest_url, &query, &se_name)) {
    // if (log_analysis::ParseSearchQuery(dest_url, &query, &se_name, &charset) &&
    //     log_analysis::IsGeneralSearch(se_name.c_str())) {
      // 展现
      std::string key = se_name + "\t" + dest_url;
      Info info;
      info.time_stamp = timestamp;
      info.query = query;
      collection.insert(std::make_pair(key, info));
    } else {
      CHECK(log_analysis::IsGeneralSearch(ref_url, &query, &se_name));
      // CHECK(log_analysis::ParseSearchQuery(ref_url, &query, &se_name, &charset) &&
      //     log_analysis::IsGeneralSearch(se_name.c_str()));
      if (log_analysis::IsAds(dest_url, NULL)) {
        continue;
      }

      // 点击
      std::string key = se_name + "\t" + ref_url;
      // 从展现的 collection 中找到时间戳 小于等于 点击时间戳 的最近展现记录
      std::multimap<std::string, Info>::iterator best_show = collection.end();
      std::string best_timestamp;
      auto range = collection.equal_range(key);
      for (auto it = range.first; it != range.second; ++it) {
        const Info& info = it->second;
        if (info.time_stamp <= timestamp &&
            (best_timestamp.empty() || info.time_stamp > best_timestamp)) {
          best_timestamp = info.time_stamp;
          best_show = it;
        }
      }
      if (best_show != collection.end()) {
        best_show->second.clicked_urls.push_back(dest_url);
      } else {
        // 允许点击没有找到任何展现(因为数据丢失)
        // 此时补一条展现数据
        // Info info;
        // info.time_stamp = timestamp;
        // info.query = query;
        // info.clicked_urls.push_back(dest_url);
        // collection.insert(std::make_pair(key, info));
      }
    }
  }
  int32 reduce_id = base::CityHash64(common_mid.c_str(), common_mid.size()) % reducer_count;
  for (auto it =  collection.begin(); it != collection.end(); ++it) {
    // key: search_engine + search_dest_url
    const std::string& key = it->first;
    flds.clear();
    base::SplitString(key, "\t", &flds);
    const std::string& se_name = flds[0];
    std::string compound_key = common_mid + "\t" + se_name;

    const Info& info = it->second;
    const std::string& time_stamp = info.time_stamp;
    const std::string& query = info.query;
    std::string clicked_urls;
    if (info.clicked_urls.size() > 0) {
      clicked_urls = base::LineEscapeString(base::JoinStrings(info.clicked_urls, "\t"));
    }
    task->OutputWithReducerID(compound_key,
                              time_stamp + "\t" + query + "\t" + clicked_urls,
                              reduce_id);
  }
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (file_name.find(FLAGS_pv_log_path) != std::string::npos) {
    input_type = 0;
  } else if (file_name.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    // LOG(FATAL) << "unknown input type, file: " << file_name;
    input_type = FLAGS_type;
  }

  std::string key;
  std::string value;
  std::string last_key;
  std::vector<std::string> buffer;
  std::vector<std::string> flds;
  if (input_type == 0) {  // pv_log
    while (task->NextKeyValue(&key, &value)) {
      // key is mid
      if (key != last_key) {
        ProcessPvLog(buffer, task);
        buffer.clear();
        std::vector<std::string> a;
        buffer.swap(a);
      }
      // 提取跟搜索相关的日志
      std::string line = key + "\t" + value;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      CHECK_EQ(flds.size(), 7u);
      const std::string& dest_url = flds[2];
      const std::string& ref_url = flds[3];
      // std::string query, se, charset;
      // bool dest_is_search = false;
      // if (log_analysis::ParseSearchQuery(dest_url, &query, &se, &charset)) {
      //   if (!log_analysis::IsGeneralSearch(se.c_str())) {
      //     // 目标页是垂直搜索
      //     continue;
      //   }
      //   dest_is_search = true;
      // }
      // bool ref_is_search = false;
      // if (log_analysis::ParseSearchQuery(ref_url, &query, &se, &charset) &&
      //     log_analysis::IsGeneralSearch(se.c_str())) {
      //   ref_is_search = true;
      // }
      // if ((!dest_is_search && ref_is_search) ||
      //     (dest_is_search)) {
      //   buffer.push_back(key + "\t" + value);
      // }

      if (log_analysis::IsGeneralSearch(dest_url, NULL, NULL)
          || log_analysis::IsGeneralSearchClick(dest_url, ref_url, NULL, NULL)) {
        // 目标页是搜索结果页, 或者是搜索点击页
        buffer.push_back(key + "\t" + value);
      }
      last_key = key;
    }
    ProcessPvLog(buffer, task);
  } else {  // search log
    while (task->NextKeyValue(&key, &value)) {
      // key is search id
      if (key != last_key) {
        ProcessSearchLog(buffer, task);
        buffer.clear();
        std::vector<std::string> a;
        buffer.swap(a);
      }
      buffer.push_back(key + "\t" + value);
      last_key = key;
    }
    ProcessSearchLog(buffer, task);
  }
}

struct SearchInfo {
  std::string time_str;
  int64       time_sec;
  std::string query;
  std::string sid;
  std::vector<std::string> rank_url;
  std::set<std::string> urls;
  int pv_info;  // for merge, default is -1
};

void BuildSearchInfo(const std::string& sename, const std::vector<std::string>& search_actions,
                     std::vector<SearchInfo>* search_info) {
  std::vector<std::string> flds;
  search_info->clear();
  for (int i = 0; i < (int)search_actions.size(); ++i) {
    flds.clear();
    base::SplitString(search_actions[i], "\t", &flds);
    search_info->push_back(SearchInfo());
    // search_actions: timestamp, query, sid, rank_url
    SearchInfo& info = search_info->back();
    info.time_str = flds[0];
    CHECK(log_analysis::ConvertTimeFromFormatToSec(info.time_str, "%Y%m%d%H%M%S", &info.time_sec));
    info.query = flds[1];
    info.sid = flds[2];
    std::string temp_str;
    CHECK(base::LineUnescape(flds[3], &temp_str));
    base::SplitString(temp_str, "\t", &(info.rank_url));
    for (int j = 1; j < (int)info.rank_url.size(); j += 2) {
      const std::string& dest_url = info.rank_url[j];
      std::string target_url;
      if (sename == log_analysis::kGoogle) {
        // 如果是 google 的结果, 需要做跳转连接识别
        target_url = log_analysis::GoogleTargetUrl(dest_url);
      } else {
        target_url = dest_url;
      }
      if ((j+1) / 2 <= 10) { 
        info.urls.insert(target_url);
      }
    }
    info.pv_info = -1;
  }
}

struct PvInfo {
  std::string time_str;
  int64       time_sec;
  std::string query;
  std::set<std::string> clicked_urls;
  int search_info;
};

void BuildPvInfo(const std::string& sename, const std::vector<std::string>& pv_actions,
                 std::vector<PvInfo>* pv_info) {
  std::vector<std::string> flds;
  pv_info->clear();
  for (int i = 0; i < (int)pv_actions.size(); ++i) {
    flds.clear();
    base::SplitString(pv_actions[i], "\t", &flds);
    pv_info->push_back(PvInfo());
    // pv_actions: timestamp, query, clicked_urls
    PvInfo& info = pv_info->back();
    info.time_str = flds[0];
    CHECK(log_analysis::ConvertTimeFromFormatToSec(info.time_str, "%Y%m%d%H%M%S", &info.time_sec));
    info.query = flds[1];
    std::string temp;
    if (!flds[2].empty()) {
      CHECK(base::LineUnescape(flds[2], &temp));
      flds.clear();
      base::SplitString(temp, "\t", &flds);
      for (int j = 0; j < (int)flds.size(); ++j) {
        const std::string& dest_url = flds[j];
        std::string target_url;
        if (sename == log_analysis::kGoogle) {
          // 如果是 google 的结果, 需要做跳转连接识别
          target_url = log_analysis::GoogleTargetUrl(dest_url);
        } else {
          target_url = dest_url;
        }
        info.clicked_urls.insert(target_url);
      }
    }
    info.search_info = -1;
  }
}

int MergeByQuery(std::vector<SearchInfo> *search_info_pointer, std::vector<PvInfo>* pv_info_pointer) {
  std::vector<SearchInfo>& search_info = *search_info_pointer;
  std::vector<PvInfo>& pv_info = *pv_info_pointer;
  if (pv_info.empty()) return 0;

  std::multimap<std::string, int>  query_pv;
  int match = 0;
  for (int i = 0; i < (int)pv_info.size(); ++i) {
    if (pv_info[i].search_info != -1) continue;
    const std::string& query = pv_info[i].query;
    if (query.empty()) continue;
    query_pv.insert(std::make_pair(query, i));
  }

  for (int i = 0; i < (int)search_info.size(); ++i) {
    SearchInfo& s_info = search_info[i];
    const std::string& query = s_info.query;
    if (query.empty()) continue;
    if (s_info.pv_info != -1) continue;

    // search log 中的 query 不为空, 以 query 为 key, 查询得到 pv log 中该 query 的记录
    auto range = query_pv.equal_range(query);
    int64 min_elapse = FLAGS_query_elapse;
    int best_id = -1;
    // 检查这些记录的时间点, 选择一个在 1 小时以内且最近的 pv log 与该 search log 对应
    for (auto it = range.first; it != range.second; ++it) {
      int pv_log_id = it->second;
      CHECK(pv_log_id >= 0 && pv_log_id < (int)pv_info.size());
      const PvInfo& p_info = pv_info[pv_log_id];
      if (p_info.search_info != -1) continue;

      // 如果 pv log 的 click 不为空, 看他们是否都出现在本条 search log 的搜索结果中
      if (!p_info.clicked_urls.empty()) {
        auto &clicks = p_info.clicked_urls;
        bool all_hit = true;
        for (auto jt = clicks.begin(); jt != clicks.end(); ++jt) {
          const std::string& url = *jt;
          if (s_info.urls.find(url) == s_info.urls.end()) {
            all_hit = false;
            break;
          }
        }
        if (!all_hit) {
          LOG(ERROR) << "merge by query but filter out by click";
          continue;
        }
      }

      int64 time_diff = labs(p_info.time_sec - s_info.time_sec);
      if (time_diff < min_elapse) {
        min_elapse = time_diff;
        best_id = pv_log_id;
      }
    }
    if (best_id != -1) {
      s_info.pv_info = best_id;
      pv_info[best_id].search_info = i;
      match++;
    }
  }
  return match;
}

int MergeByClick(std::vector<SearchInfo> *search_info_pointer, std::vector<PvInfo>* pv_info_pointer) {
  std::vector<SearchInfo>& search_info = *search_info_pointer;
  std::vector<PvInfo>& pv_info = *pv_info_pointer;
  if (pv_info.empty()) return 0;

  int match = 0;
  for (int i = 0; i < (int)pv_info.size(); ++i) {
    if (pv_info[i].search_info != -1) continue;
    PvInfo& p_info = pv_info[i];

    const auto& clicks = p_info.clicked_urls;
    if (clicks.empty()) continue;

    int64 min_elapse = FLAGS_click_elapse;
    int best_id = -1;
    for (int j = 0; j < (int)search_info.size(); ++j) {
      SearchInfo& s_info = search_info[j];
      if (s_info.pv_info != -1) continue;

      bool all_hit = true;
      for (auto it = clicks.begin(); it != clicks.end(); ++it) {
        const std::string& url = *it;
        if (s_info.urls.find(url) == s_info.urls.end()) {
          all_hit = false;
          break;
        }
      }

      if (all_hit) {
        // LOG(INFO) << "merge by click";
        int64 time_diff = labs(s_info.time_sec - p_info.time_sec);
        if (time_diff < min_elapse) {
          min_elapse = time_diff;
          best_id = j;
        }
      }
    }

    if (best_id != -1) {
      p_info.search_info = best_id;
      search_info[best_id].pv_info = i;
      match++;
    }
  }
  return match;
}

int MergeByTime(std::vector<SearchInfo> *search_info_pointer, std::vector<PvInfo>* pv_info_pointer) {
  std::vector<SearchInfo>& search_info = *search_info_pointer;
  std::vector<PvInfo>& pv_info = *pv_info_pointer;
  if (pv_info.empty()) return 0;

  std::multimap<int64, int>  time_pv;
  int match = 0;
  for (int i = 0; i < (int)pv_info.size(); ++i) {
    if (pv_info[i].search_info != -1) continue;
    const int64& time = pv_info[i].time_sec;
    time_pv.insert(std::make_pair(time, i));
  }

  for (int i = 0; i < (int)search_info.size(); ++i) {
    SearchInfo& s_info = search_info[i];
    const int64& s_time = s_info.time_sec;
    if (s_info.pv_info != -1) continue;

    int64 min_elapse = FLAGS_elapse;
    int best_id = -1;
    for (auto it = time_pv.begin(); it != time_pv.end(); ++it) {
      const int64& p_time = it->first;
      if (p_time > s_time + FLAGS_elapse) break;
      int pv_index = it->second;
      if (pv_info[pv_index].search_info != -1) continue;
      int64 time_diff = labs(p_time - s_time);
      if (time_diff > FLAGS_elapse) continue;
      if (time_diff < min_elapse) {
        min_elapse = time_diff;
        best_id = it->second;
      }
    }

    if (best_id != -1) {
      s_info.pv_info = best_id;
      pv_info[best_id].search_info = i;
      match++;
    }
  }
  return match;
}

void MergeLogs(const std::string& key,
               const std::vector<std::string>& search_actions,
               const std::vector<std::string>& pv_actions,
               base::mapreduce::ReduceTask *task) {
  // key: mid + search engin
  // search_actions: timestamp, query, sid, specific_info
  // pv_actions: timestamp, query, clicked_urls

  if (search_actions.empty()) return;

  std::vector<std::string> flds;
  flds.clear();
  base::SplitString(key, "\t", &flds);
  std::string mid = flds[0];
  std::string sename = flds[1];

  std::vector<SearchInfo> search_info;
  BuildSearchInfo(sename, search_actions, &search_info);
  std::vector<PvInfo> pv_info;
  BuildPvInfo(sename, pv_actions, &pv_info);

  int match_click = 0;
  match_click = MergeByClick(&search_info, &pv_info);

  int match_query = 0;
  match_query = MergeByQuery(&search_info, &pv_info);

  int match_time = 0;
  match_time = MergeByTime(&search_info, &pv_info);

  LOG(INFO) << base::StringPrintf("total %d to merge: match %d by click, %d by query, %d by time",
                                  (int)search_actions.size(), match_click, match_query, match_time);

  // search_actions: timestamp -> query, sid, rank_url
  // pv_actions: timestamp -> query, clicked_urls
  // key: mid + search engin
  std::map<std::string, std::string> search_log_buffer;
  for (int i = 0; i < (int)search_info.size(); ++i) {
    SearchInfo& s_info = search_info[i];
    const PvInfo* p_info = NULL;
    if (s_info.pv_info != -1) {
      p_info = &(pv_info[s_info.pv_info]);
      if (s_info.query != p_info->query &&
          !p_info->query.empty()) {
        s_info.query = p_info->query;
      }
    }
    for (int j = 0; j < (int)s_info.rank_url.size(); j += 2) {
      // 输出 search log
      std::string key = mid + "\t" + s_info.time_str + "\t" +
          base::StringPrintf("%2d", base::ParseIntOrDie(s_info.rank_url[j]));
      std::string value = s_info.sid + "\t" +
                   mid + "\t" + s_info.time_str + "\t" +
                   s_info.query + "\t"+ sename + "\t" +
                   s_info.rank_url[j] + "\t" + s_info.rank_url[j+1];
      search_log_buffer[key] = value;

      if (p_info == NULL) continue;
      if (s_info.query.empty()) continue;
      // 输出 show click
      std::string url = s_info.rank_url[j+1];
      if (sename == log_analysis::kGoogle) {
        url = log_analysis::GoogleTargetUrl(s_info.rank_url[j+1]);
      }
      std::string click = "0";
      if (p_info->clicked_urls.find(url) != p_info->clicked_urls.end()) {
        click = "1";
      }
      // query url   se rank show click 
      task->OutputWithFilePrefix(s_info.query,
                                 url + "\t" + sename + "\t" + s_info.rank_url[j] + "\t1\t" + click,
                                 "show_click");
    }
  }

  for (auto it = search_log_buffer.begin(); it != search_log_buffer.end(); ++it) {
    const std::string& line = it->second;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    task->OutputWithFilePrefix(flds[0], log_analysis::JoinFields(flds, "\t", "123456"), "search_log");
  }
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();

  std::string key;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    std::vector<std::string> search_actions;
    std::vector<std::string> pv_actions;

    // 每个桶搜集了一个用户在某个搜索引擎上的所有行为
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.size() == 4) {  // search log
        // timestamp -> query, sid, specific_info
        search_actions.push_back(value);
      } else if (flds.size() == 3) {  // show,click in pv log
        // timestamp -> query, clicked_urls
        pv_actions.push_back(value);
      } else {
        CHECK(false) << value;
      }
    }

    MergeLogs(key, search_actions, pv_actions, task);
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log r1");
  if (base::mapreduce::IsMapApp()) {
    CHECK(!FLAGS_pv_log_path.empty() && !FLAGS_search_log_path.empty())
        << "specify md5_url_path and search_log_path";
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
