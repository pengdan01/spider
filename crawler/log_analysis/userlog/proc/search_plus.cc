#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <map>
#include <set>

#include "log_analysis/userlog/proc/common.h"
#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/base64.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/timestamp.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "base/hash_function/md5.h"
#include "nlp/common/nlp_util.h"

bool ParseQuery(const std::map<int, std::string>& kv_info,
                base::mapreduce::MapTask *task,
                std::string* search_engine,
                std::string* query,
                std::string* url) {
  url->clear();
  query->clear();
  if ((kv_info.find(kIDSearchKeyword) == kv_info.end() 
       || kv_info.find(kIDSearchKeyword)->second.empty())
       && (kv_info.find(kIDSearchResultWebPageURL) == kv_info.end()
          || kv_info.find(kIDSearchResultWebPageURL)->second.empty())) {
    return false;
  }

  std::string debug_info;
  std::string search_query1;
  std::string search_query2;

  auto it = kv_info.find(kIDSearchKeyword);
  if (it != kv_info.end() && !it->second.empty()) {
    if (debug_info.empty()) debug_info += "\t";
    debug_info += base::StringPrintf("Keyword: [%s]", it->second.c_str());
    const std::string& encoded_query = it->second;
    if (!log_analysis::Base64ToUTF8(encoded_query, &search_query1)) {
      search_query1.clear();
    } else {
      nlp::util::NormalizeLineInPlaceS(&search_query1);
    }
  }

  it = kv_info.find(kIDSearchResultWebPageURL);
  if (it != kv_info.end() && !it->second.empty()) {
    if (debug_info.empty()) debug_info += "\t";
    debug_info += base::StringPrintf("Url: [%s]", it->second.c_str());
    const std::string& encoded_url = it->second;
    std::string click_url;
    std::string parsed_engine;
    if (log_analysis::Base64ToClickUrl(encoded_url, &click_url)) {
      *url = click_url;
      if (log_analysis::IsGeneralSearch(click_url, &search_query2, &parsed_engine)) {
        if (parsed_engine != *search_engine) {
          // 发现过传 soso 的 url, 说是 baidu 的结果
          LOG(ERROR) << "change search engine: " << *search_engine << "->" << parsed_engine;
          *search_engine = parsed_engine;
        }
      } else {
        task->ReportAbnormalData("ParseQueryFailed", click_url, encoded_url);
        search_query2.clear();
      }
    } else {
      LOG(ERROR) << "failed to parse url: " << encoded_url;
      search_query2.clear();
    }
  }

  if (search_query1.empty() && search_query2.empty()) {
    return false;
  }

  if (!search_query1.empty()) {
    LOG_IF(FATAL, !search_query2.empty() && search_query1 != search_query2) << debug_info;
    *query = search_query1;
  }
  if (!search_query2.empty()) {
    LOG_IF(FATAL, !search_query1.empty() && search_query1 != search_query2) << debug_info;
    *query = search_query2;
  }
  return true;
}

bool ParseSearchResult(const std::map<int, std::string>& kv_info,
                       const std::string& line,
                       std::vector<std::string>* result_urls,
                       std::vector<std::string>* result_titles) {
  auto it = kv_info.find(kIDSearchResultURL);
  LOG_IF(FATAL, it != kv_info.end()) << it->second;
  it = kv_info.find(kIDSearchResultURLMD5);
  if (it == kv_info.end()) {
    return false;
  }
  std::vector<std::string> flds;
  base::SplitString(it->second, ":", &flds);
  result_urls->clear();
  result_titles->clear();
  result_urls->reserve(flds.size());
  result_titles->reserve(flds.size());
  std::vector<std::string> subs;
  for (int i = 0; i < (int)flds.size(); ++i) {
    if (flds[i].empty()) continue;
    const std::string& field = flds[i];
    subs.clear();
    base::SplitString(field, ",", &subs);
    if (subs.size() < 2) continue;
    std::string url, title;
    if (!subs[1].empty() && log_analysis::Base64ToClickUrl(subs[1], &url)) {
      result_urls->push_back(url);
    } else {
      result_urls->push_back(std::string());
    }

    if (subs.size() > 2 && !subs[2].empty() && log_analysis::Base64ToUTF8(subs[2], &title)) {
      result_titles->push_back(title);
    } else {
      result_titles->push_back(std::string());
    }
  }
  return true;
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK(base::mapreduce::IsMapApp());

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  std::unordered_map<std::string, std::string> se_map;
  // kSEQihoo   = 0,
  // kSEGoogle  = 1,
  // kSEBaidu   = 2,
  // kSEBing    = 3,
  // kSEYahoo   = 4,
  // kSESogou   = 5,   
  // kSEYoudao  = 6,   
  // kSESoso    = 7,
  // kSEUnknown = 0xFF
  se_map["0"] = "Qihoo";
  se_map["1"] = log_analysis::kGoogle;
  se_map["2"] = log_analysis::kBaidu;
  se_map["3"] = log_analysis::kBing;
  se_map["4"] = log_analysis::kYahoo;
  se_map["5"] = log_analysis::kSogou;
  se_map["6"] = log_analysis::kYoudao;
  se_map["7"] = log_analysis::kSoso;
  se_map["255"] = "Unknown";

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  std::vector<std::string> result_urls;
  std::vector<std::string> result_titles;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (flds.size() < 9) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string mid = flds[0];
    if (mid.empty()) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    {
      std::string url_info = flds[6];
      std::string ref_info = flds[7];
      if (!url_info.empty() || !ref_info.empty()) {
        task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      }
    }
    std::string search_info = flds[8];

    std::string server_time = flds[4];
    if (!log_analysis::ConvertTimeFromSecToFormat(flds[4], "%Y%m%d%H%M%S", &server_time)) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string client_ip;
    if (flds.size() >= 14) {
      client_ip = flds[13];
    }

    bool abnormal = false;
    std::map<int, std::string> kv_info;
    if (!search_info.empty()) {
      flds.clear();
      base::SplitString(search_info, ";", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], "|", &sub);
        if (sub.size() != 2u) {
          LOG(ERROR)  << base::StringPrintf("invalid field, line: [%s], search_info: [%s], field: [%s]",
                                  line.c_str(), search_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        int key = 0;
        if (!base::StringToInt(sub[0], &key)) {
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        if (kv_info.find(key) != kv_info.end()
            && kv_info[key] != sub[1]) {
          LOG(ERROR) << base::StringPrintf("duplicated key, line: [%s], search_info: [%s], field: [%s]",
                                  line.c_str(), search_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        kv_info[key] = sub[1];
      }
    }
    if (abnormal) continue;

    std::string se_id;
    auto it = kv_info.find(kIDSearchEngineType);
    if (it == kv_info.end() || !decode_search_type(it->second, &se_id)) {
      se_id = -1;
    }
    std::string se_type = "Unknown";
    if (se_map.find(se_id) != se_map.end()) {
      se_type = se_map[se_id];
    }

    std::string query;
    std::string search_url;
    if (!ParseQuery(kv_info, task, &se_type, &query, &search_url)) {
      query.clear();
      search_url.clear();
    }

    if (!ParseSearchResult(kv_info, line, &result_urls, &result_titles)) {
      task->ReportAbnormalData(log_analysis::kInvalidURL, line, "");
      continue;
    }
    CHECK_EQ(result_titles.size(), result_urls.size());
    std::string merged_results;
    for (int i = 0; i < (int)result_titles.size(); ++i) {
      if (!result_urls[i].empty() && !result_titles[i].empty()) {
        task->Output(result_urls[i], "title\t" + base::LineEscapeString(result_titles[i]));
      }
      if (!result_urls[i].empty()) {
        if (!merged_results.empty()) merged_results += "\t";
        merged_results += base::StringPrintf("%d\t%s", i+1, result_urls[i].c_str());
      }
    }

    if (!merged_results.empty()) {
      int32 reduce_id = base::CityHash64(mid.c_str(), mid.size()) % reducer_count;
      std::string compound_key = mid + "\t" + server_time + "\t" +
          se_type + "\t" + base::LineEscapeString(search_url) + "\t" +
          client_ip + "\t" +base::LineEscapeString(query);
      task->OutputWithReducerID(compound_key, "searchlog\t" + base::LineEscapeString(merged_results), reduce_id);
    }
  }
}

void reducer() {
  base::PseudoRandom random(base::GetTimestamp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::vector<std::string> flds;
  std::set<std::string> buffer;
  while (task->NextKey(&key)) {
    std::string value;
    buffer.clear();
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      CHECK_EQ(flds.size(), 2u);
      if (flds.front() == "title") {
        // title 不做 LineUnescape
        task->OutputWithFilePrefix(key, flds[1], "title_search");
      } else if (flds.front() == "searchlog") {
        std::string decoded;
        CHECK(base::LineUnescape(flds[1], &decoded));
        buffer.insert(decoded);
      } else {
        CHECK(false);
      }
    }
    if (buffer.empty()) continue;
    if (buffer.size() != 1u) {
      LOG(ERROR) << "too much value: " << key;
      continue;
    }
    const std::string& merged_results = *buffer.begin();

    flds.clear();
    base::SplitString(key, "\t", &flds);
    CHECK_EQ(flds.size(), 6u);
    std::string mid = flds[0];
    std::string time_stamp = flds[1];
    std::string se_type = flds[2];
    std::string search_url;
    CHECK(base::LineUnescape(flds[3], &search_url));
    std::string client_ip = flds[4];
    std::string query;
    CHECK(base::LineUnescape(flds[5], &query));
    std::string sid = MD5String(base::LineEscapeString(mid + "\t" +
                                                       se_type + "\t" +
                                                       search_url + "\t" +
                                                       time_stamp + "\t" +
                                                       random.GetString(10)));

    flds.clear();
    base::SplitString(merged_results, "\t", &flds);
    CHECK_EQ(flds.size() % 2, 0u) << merged_results;
    for (int i = 0; i < (int)flds.size(); i += 2) {
      const std::string& rank = flds[i];
      const std::string& url = flds[i+1];
      task->OutputWithFilePrefix(sid,
                                 mid + "\t" +
                                 time_stamp + "\t" +
                                 query + "\t" +
                                 se_type + "\t" +
                                 rank + "\t" +
                                 url + "\t" +
                                 search_url + "\t" +
                                 client_ip, "searchlog");
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search v2");
  // base::InitApp(&argc, &argv, "pv v2");
  // mapper();
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
