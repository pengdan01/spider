// 通过 url 对应的 query 出现次数，投票选出 search log 对应的 query
#include <set>
#include <queue>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/encoding/line_escape.h"
#include "base/mapreduce/mapreduce.h"

#include "log_analysis/common/log_common.h"

DEFINE_string(search_log_path, "", "hdfs path of search_log");
DEFINE_string(url_query_path,  "", "hdfs path of url query");
DEFINE_string(sid_query_path,  "", "hdfs path of sid query");
DEFINE_int32(round, -1, "which round");
DEFINE_int32(top_n, 300, "top n");
DEFINE_int32(min_hit_num, 6, "the min urls a query hit");

void mapper_r1() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  int input_type;
  if (hdfs_path.find(FLAGS_search_log_path) != std::string::npos) {  // search log
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_url_query_path) != std::string::npos) {  // url query
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << hdfs_path;
  }

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    line = key + "\t" + value;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (input_type == 0) {  //  search log
      DCHECK_EQ(flds.size(), 7u) << line;
      const std::string& sid = flds[0];
      const std::string& query = flds[3];
      const std::string& se_name = flds[4];
      const std::string& url = flds[6];
      // query 非空, 无需还原
      if (!query.empty()) continue;

      // rank 大于10的是广告, 不考虑
      int rank = base::ParseIntOrDie(flds[5]);
      if (rank > 10) continue;

      std::string target_url;
      if (se_name == log_analysis::kGoogle) {
        // 如果是 google 的结果, 需要做跳转连接识别
        target_url = log_analysis::GoogleTargetUrl(url);
      } else {
        target_url = url;
      }
      // key: url se_name,  value: sid
      task->Output(target_url + "\t" + se_name, sid + "\tS");
    } else {  // url sename query count
      DCHECK_EQ(flds.size(), 4u) << line;
      task->Output(flds[0] + "\t" + flds[1],
                   flds[2] + "\t" + flds[3] + "\tQ");
    }
  }
}

void reducer_r1() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::priority_queue<std::pair<int64, std::string>> top_n_queries;
    std::vector<std::string> sids;
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.size() == 3 && flds.back() == "Q") { // queries
        std::string query = flds[0];
        int64 count = base::ParseInt64OrDie(flds[1]);
        top_n_queries.push(std::make_pair(count * -1, query));  // 最大堆变最小堆
        if ((int)top_n_queries.size() > FLAGS_top_n) {
          top_n_queries.pop();
        }
      } else if (flds.size() == 2 && flds.back() == "S") {  // search log
        sids.push_back(flds[0]);
      } else {
        CHECK(false) << value;
      }
    }

    // query 或者 sid 为空, 都不需要输出
    if (top_n_queries.empty() || sids.empty()) {
      continue;
    }

    // 获取堆中所有 query. 不需要输出频次
    std::vector<std::string> queries;
    while (!top_n_queries.empty()) {
      queries.push_back(top_n_queries.top().second);
      top_n_queries.pop();
    }

    std::string str_queries = base::JoinStrings(queries, "\t");
    for (int i = 0; i < (int)sids.size(); ++i) {
      const std::string& sid = sids[i];
      task->Output(sid, str_queries);
    }
  }
}

void ProcessSearchLog(const std::vector<std::string>& buffer,
                      base::mapreduce::MapTask *task) {
  if (buffer.size() == 0) return;

  std::vector<std::string> flds;
  const std::string& line = buffer[0];
  flds.clear();
  base::SplitString(line, "\t", &flds);
  CHECK_EQ(flds.size(), 7u);
  // std::string sid = flds[0];
  // std::string mid = flds[1];
  // std::string timestamp = flds[2];
  // std::string query = flds[3];
  // std::string sename = flds[4];
  std::string key = flds[0];
  std::string common_info = base::LineEscapeString(log_analysis::JoinFields(flds, "\t", "01234"));

  std::vector<std::string> rank_url;
  for (int i = 0; i < (int)buffer.size(); ++i) {
    const std::string& line = buffer[i];
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 7u);
    CHECK_EQ(flds[0], key);
    const std::string& rank = flds[5];
    const std::string& url = flds[6];

    rank_url.push_back(rank + "\t" + url);
  }
  
  // 将多条 search log "打包"输出, 这样能够保证在 shuffle 过程中这些记录的顺序不会改变
  task->Output(key,
               common_info + "\t" +
               base::LineEscapeString(base::JoinStrings(rank_url, "\t")) + "\tS");
}

void mapper_r2() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  int input_type;
  if (hdfs_path.find(FLAGS_search_log_path) != std::string::npos) {  // search log
    input_type = 0;
  } else if (hdfs_path.find(FLAGS_sid_query_path) != std::string::npos) {  // sid query
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << hdfs_path;
  }

  std::string key;
  std::string value;
  std::string last_key;
  std::vector<std::string> buffer;
  if (input_type == 0) {  //  search log
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
  } else {  // sid query
    while (task->NextKeyValue(&key, &value)) {
      task->Output(key, value);
    }
  }
}

void OutputWithQuerySelection(const std::string& common_info,
                              const std::string& str_rank_urls,
                              const std::map<std::string, int>& query_counts,
                              base::mapreduce::ReduceTask *task) {
  std::string best_query;
  int best_count = -1;
  for (auto it = query_counts.begin(); it != query_counts.end(); ++it) {
    if (it->second > best_count) {
      best_count = it->second;
      best_query = it->first;
    }
  }
  if (best_count < FLAGS_min_hit_num) {
    best_query.clear();
  }

  std::vector<std::string> rank_urls;
  std::vector<std::string> flds;
  std::string temp;
  CHECK(base::LineUnescape(str_rank_urls, &temp));
  base::SplitString(temp, "\t", &rank_urls);
  CHECK_EQ(rank_urls.size() % 2, 0u);

  temp.clear();
  CHECK(base::LineUnescape(common_info, &temp));
  flds.clear();
  base::SplitString(temp, "\t", &flds);
  // std::string query = flds[3];
  if (flds[3].empty() && !best_query.empty()) {
    flds[3] = best_query;
  }
  std::string new_common_info = base::JoinStrings(flds, "\t");

  for (int i = 0; i < (int)rank_urls.size(); i += 2) {
    const std::string& rank = rank_urls[i];
    const std::string& url  = rank_urls[i+1];
    task->Output(new_common_info, rank + "\t" + url);
  }
}

void reducer_r2() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key, value;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    std::map<std::string, int> common_query_counts;
    std::vector<std::pair<std::string, std::string>> search_log;
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      if (flds.size() == 3 && flds.back() == "S") { // search log
        search_log.push_back(std::make_pair(flds[0], flds[1]));
      } else { // queries 
        for (int i = 0; i < (int)flds.size(); ++i) {
          common_query_counts[flds[i]] += 1;
        }
      }
    }

    CHECK_EQ(search_log.size(), 1u) << "must has only 1 sid" << key;
    OutputWithQuerySelection(search_log[0].first, search_log[0].second,
                            common_query_counts, task);
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "search log query match");
  if (base::mapreduce::IsMapApp()) {
    if (FLAGS_round == 1) {
      CHECK(!FLAGS_search_log_path.empty() && !FLAGS_url_query_path.empty())
          << "specify search_log path and url_query path";
      mapper_r1();
    } else if (FLAGS_round == 2) {
      CHECK(!FLAGS_search_log_path.empty() && !FLAGS_sid_query_path.empty())
          << "specify search_log path and url_query path";
      mapper_r2();
    } else {
      CHECK(false);
    }
  } else if (base::mapreduce::IsReduceApp()) {
    if (FLAGS_round == 1) {
      reducer_r1();
    } else if (FLAGS_round == 2) {
      reducer_r2();
    } else {
      CHECK(false);
    }
  } else {
    CHECK(false);
  }
  return 0;
}

