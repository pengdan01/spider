#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/time/timestamp.h"
#include "base/hash_function/md5.h"
#include "base/encoding/line_escape.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

DEFINE_string(md5_url_path, "", "hdfs path of md5->url files");
DEFINE_string(search_log_path,  "", "hdfs path of pv log");

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::PseudoRandom rand(base::GetTimestamp());

  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (file_name.find(FLAGS_md5_url_path) != std::string::npos) {
    input_type = 0;
  } else if (file_name.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << file_name;
  }

  std::unordered_map<std::string, std::string> se_map;
  se_map["0"] = "Qihoo";
  se_map["1"] = log_analysis::kGoogle;
  se_map["2"] = log_analysis::kBaidu;
  se_map["3"] = log_analysis::kBing;
  se_map["4"] = log_analysis::kYahoo;
  se_map["5"] = log_analysis::kSogou;
  se_map["6"] = "Youdao";
  se_map["7"] = log_analysis::kSoso;
  se_map["255"] = "Unknown";

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  std::vector<std::string> urls;
  std::string last_mid;
  std::string last_timestamp;
  int last_rand_num = -1;

  int line_count = 0;
  while (task->NextKeyValue(&key, &value)) {
    line_count++;
    if (input_type == 1) {
      line = key + "\t" + value;
      if (line.empty()) continue;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      // mid se_type query_md5 url_md5 time_stamp
      if (flds.size() < 5) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      std::string mid = flds[0];
      if (mid.empty()) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      std::string se_type = "Unknown";
      if (se_map.find(flds[1]) != se_map.end()) {
        se_type = se_map[flds[1]];
      }

      std::string query_md5 = flds[2];
      if (flds.size() >= 7) {
        query_md5 = flds[6];
      } else {
        query_md5.clear();
      }

      std::string time_stamp;
      if (!log_analysis::ConvertTimeFromSecToFormat(flds[4], "%Y%m%d%H%M%S", &time_stamp)) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }

      std::string url_md5 = flds[3];
      urls.clear();
      base::SplitString(url_md5, "|", &urls);
      if (urls.size() == 0) {
        task->ReportAbnormalData(log_analysis::kInvalidURL, key, value);
        continue;
      }

      // 加入随机串, 保证每条日志都是一个独立的 sid
      int rand_num = line_count % 900;  // [0-899]
      // 只对 900 取模是保证有 100 个数的空间可供++
      if (mid == last_mid && time_stamp == last_timestamp) {
        // 连续两条同一用户同一时间的搜索, 需要按先后顺序输出
        rand_num = last_rand_num + 1;
      }
      std::string sid = base::LineEscapeString(mid + "\t" +
                                               se_type + "\t" +
                                               query_md5 + "\t" +
                                               time_stamp + "\t" +
                                               base::StringPrintf("%03d", rand_num));
      last_mid = mid;
      last_timestamp = time_stamp;
      last_rand_num = rand_num;

      // 如果需要拼接 query, 输出 query md5
      if (!query_md5.empty()) {
        // 塞进去一个 1 仅仅是为了不与 md5 url 的映射字段数一样
        LOG_EVERY_N(INFO, 10) << "for query" << query_md5;
        task->Output(query_md5, sid + "\tForQuery");
      }

      int rank = 1;
      for (int j = 0; j < (int)urls.size(); ++j) {
        if (urls[j].empty()) continue;
        task->Output(urls[j], sid + "\t" + base::StringPrintf("%02d", rank) + "\tForUrl");
        rank += 1;
      }
    } else {
      const std::string& base64 = key;
      std::string click_url;
      if (!log_analysis::Base64ToClickUrl(base64, &click_url)) {
        continue;
      }
      flds.clear();
      base::SplitString(value, "\t", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], ":", &sub);
        // md5, time
        CHECK_EQ(sub.size(), 2u);
        // md5, url, time
        task->Output(sub[0], click_url + "\t" + sub[1]);
      }
    }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    std::map<std::string, std::string> url_candidates;

    std::unordered_set<std::string> dedup_query;
    std::unordered_set<std::string> dedup_url;

    while (task->NextValue(&value)) {
      line = key + "\t" + value;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      if (flds.size() == 3 && flds.back() != "ForQuery") {  // md5->url
        // md5, url, time
        // 按时间戳从小到大排序 url
        url_candidates[flds[2]] = flds[1];
      } else {
        // 判断最后一个字段类型
        const std::string& type = flds.back();
        if (type == "ForQuery") {
          dedup_query.insert(line);
        } else if (type == "ForUrl") {
          dedup_url.insert(line);
        } else {
          CHECK(false);
        }
      }
    }

    std::string url;
    if (!url_candidates.empty()) {
      // 选择时间戳最靠后的记录
      url = url_candidates.rbegin()->second;
      
      if (url_candidates.size() > 1u) {
      // 出现 url 冲突
      // 报告 url 冲突
      std::string debug_str;
      for (auto it = url_candidates.begin(); it != url_candidates.end(); ++it) {
        if (!debug_str.empty()) {
          debug_str += "\t";
        }
        debug_str = debug_str + it->second + ":" + it->first;
      }
      task->ReportAbnormalData(log_analysis::kMD5Conflict, key, debug_str);
      }
    }

    if (url.empty()) {
      // 需要查询 query/url 但是没有获取 md5 对应的字面
      task->ReportAbnormalData(log_analysis::kInvalidURL, key, "");
      continue;
    }

    if (dedup_query.size() > 0) {
      std::string query, site;
      if (log_analysis::IsGeneralSearch(url, &query, &site)) {
        CHECK(!query.empty());
        for (auto it = dedup_query.begin(); it != dedup_query.end(); ++it) {
          flds.clear();
          base::SplitString(*it, "\t", &flds);
          LOG_IF(FATAL, flds.size() != 3 || flds.back() != "ForQuery")
              << base::StringPrintf("invalid line: [%s], field [%d]",
                                    it->c_str(), (int)flds.size());

          const std::string& sid = flds[1];
          // sid, query, "Query"
          task->Output(sid, query + "\tQuery");
        }
      } else {
        task->ReportAbnormalData("ParseQueryFail", url, "");
      }
    }

    for (auto it = dedup_url.begin(); it != dedup_url.end(); ++it) {
      flds.clear();
      base::SplitString(*it, "\t", &flds);
      LOG_IF(FATAL, flds.size() != 4 || flds.back() != "ForUrl")
          << base::StringPrintf("invalid line: [%s], field [%d]",
                                it->c_str(), (int)flds.size());

      const std::string& sid = flds[1];
      const std::string& rank = flds[2];
      // sid, rank, url, "Url"
      task->Output(sid, rank + "\t" + url + "\tUrl");
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log r1");
  CHECK(!FLAGS_md5_url_path.empty() && !FLAGS_search_log_path.empty())
      << "specify md5_url_path and search_log_path";
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
