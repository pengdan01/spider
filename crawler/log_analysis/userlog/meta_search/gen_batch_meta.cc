// 找出一个 url 命中的 query 以及命中次数
#include <set>
#include <map>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <vector>
#include <utility>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/mapreduce/mapreduce.h"
#include "log_analysis/common/log_common.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_printf.h"

DEFINE_int32(top_n, 200, "");

// 按 rank 加权, rank 为 1 最大 10, rank 为 10 权重 2, 大于 10 权重 1, 
inline int get_rank_weight(int rank) {
  int score = 12 - rank;
  if (score < 1) score = 1;
  return score;
}

// 把 rank 数字转换成等宽的 rank 字符串, 用于排序
inline std::string equal_length_rank(const std::string& orig_rank) {
  int rank_num = base::ParseInt32OrDie(orig_rank);
  return base::StringPrintf("%03d", rank_num);
}

static const std::string kBaidu = log_analysis::kBaidu;
static const std::string kGoogle = log_analysis::kGoogle;
static const std::string kOther = "Other";

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  std::string key;
  std::string value;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    std::string query;
    std::string se;
    std::string rank;
    std::string url;
    std::string count;

    flds.clear();
    base::SplitString(value, "\t", &flds);
    if (flds.size() >= 6) {  // search log

      query = flds[2];
      if (query.empty()) continue;

      se = flds[3];
      if (se.empty()) continue;
      if (se == log_analysis::kBaidu) {
        se = kBaidu;
      } else if (se == log_analysis::kGoogle) {
        se = kGoogle;
      } else {
        se = kOther;
      }

      rank = equal_length_rank(flds[4]);
      url = flds[5];
      if (url.empty()) continue;
      count = "1";
    } else if (flds.size() == 4) {  // history data
      // value: se rank url count
      se  = flds[0];
      rank = equal_length_rank(flds[1]);
      url = flds[2];
      count = flds[3];
    } else {
      continue;
    }
    if (query.empty() || rank.empty() || se.empty() || url.empty()) {
      CHECK(false) << key << "\t" << value;
    }
    std::string combine_key = query + "\t" + se;
    // 相同 query 进同一个桶
    int32 reduce_id = base::CityHash64(query.c_str(), query.size()) % reducer_count;
    // query  se,  rank  url  count
    task->OutputWithReducerID(combine_key, rank + "\t" + url + "\t" + count, reduce_id);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key,value;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    flds.clear();
    base::SplitString(key, "\t", &flds);
    CHECK(flds.size() == 2u);
    // query se
    std::string query = flds[0];
    std::string se = flds[1];
    std::unordered_map<std::string, uint64> url_scores;
    url_scores.clear();
    while (task->NextValue(&value)) {
      flds.clear();
      base::SplitString(value, "\t", &flds);
      CHECK_EQ(flds.size(), 3u);

      // rank url count

      int rank = base::ParseInt32OrDie(flds[0]);
      std::string url = flds[1];
      int64 count = base::ParseInt64OrDie(flds[2]);

      // 按 rank 加权, rank 为 1 最大 10, rank 为 10 权重 2, 大于 10 权重 1, 
      int64 score = get_rank_weight(rank) * count;
      // 按照 url 在相应 rank 上出现的次数重复加权
      url_scores[url] += score;
    }
    if (url_scores.empty()) continue;

    // 按照 累积权重 排序, 输出 topn
    std::vector<std::pair<uint64, std::string>> top_n;
    top_n.reserve(url_scores.size());
    for (auto it = url_scores.begin(); it != url_scores.end(); ++it) {
      top_n.push_back(std::make_pair(it->second, it->first));
    }

    if ((int)top_n.size() > FLAGS_top_n) {
      std::partial_sort(top_n.begin(), top_n.begin() + FLAGS_top_n, top_n.end(),
                        std::greater<std::pair<uint64, std::string>>());
    } else {
      std::sort(top_n.begin(), top_n.end(),
                std::greater<std::pair<uint64, std::string>>());
    }

    for (int i = 0; i < (int)top_n.size(); ++i) {
      int rank = i + 1;
      // count 按照 weight 估算
      int64 count = top_n[i].first / get_rank_weight(rank);
      // query se rank url count
      task->Output(query,
                   se + "\t"
                   + base::IntToString(rank) + "\t"
                   + top_n[i].second + "\t"
                   + base::Int64ToString(count));
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url_query_count");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}

