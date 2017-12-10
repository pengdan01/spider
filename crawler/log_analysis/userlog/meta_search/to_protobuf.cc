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
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/hash_function/term.h"
#include "base/hash_function/url.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_printf.h"

#include "log_analysis/common/log_common.h"
#include "query/parser2/query_parser.h"
#include "query/dicts/query_dicts.h"

#include "web/url_norm/url_norm.h"
#include "web/url_category/url_categorizer.h"
#include "log_analysis/userlog/meta_search/meta.pb.h"

DEFINE_int32(top_n, 50, "");

// 按 rank 加权, rank 为 1 最大 10, rank 为 10 权重 2, 大于 10 权重 1, 
inline int get_rank_weight(int rank) {
  int score = 12 - rank;
  if (score < 1) score = 1;
  return score;
}

static const std::string kBaidu = log_analysis::kBaidu;
static const std::string kGoogle = log_analysis::kGoogle;
static const std::string kOther = "Other";

// 把 rank 数字转换成等宽的 rank 字符串, 用于排序
inline std::string equal_length_rank(const std::string& orig_rank) {
  int rank_num = base::ParseInt32OrDie(orig_rank);
  return base::StringPrintf("%03d", rank_num);
}

void proc(const std::string& query, const std::vector<std::string>& records,
          queries::QueryParser* parser, web::UrlNorm* url_norm,
  web::util::UrlCategorizer* url_class,
          base::mapreduce::MapTask *task) {
  std::string norm_query;
  if (!queries::NormalizeQueryS(query, parser, &norm_query)) {
    return;
  }

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);
  // 相同 query 进同一个桶
  int32 reduce_id = base::CityHash64(query.c_str(), query.size()) % reducer_count;

  std::vector<std::string> flds;
  for (size_t i = 0; i < records.size(); ++i) {
    flds.clear();
    base::SplitString(records[i], "\t", &flds);
    if (flds.size() != 4u) continue;

    // se rank url count
    const std::string& se = flds[0];
    const std::string& rank = flds[1];
    std::string url = flds[2];
    if (se == kGoogle) {
      url = log_analysis::GoogleTargetUrl(url);
    }
    std::string norm_url;
    if (!url_norm->NormalizeClickUrl(url, &norm_url)) {
      continue;
    }
    int url_type;
    std::string dump;
    if (!url_class->Category(norm_url, &url_type, &dump)) {
      continue;
    }
    if (url_type < web::util::kNormal) {
      LOG(INFO) << "useless url: " << norm_url;
      continue;
    }
    const std::string& count = flds[3];

    task->OutputWithReducerID(norm_query + "\t" + se,
                              rank + "\t" + url + "\t" + count, reduce_id);
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  queries::QueryParser parser;
  queries::QueryDicts  dicts;
  dicts.LoadAllDicts();
  parser.SetDicts(&dicts);
  web::UrlNorm url_norm;
  web::util::UrlCategorizer url_class;

  std::string last_key;
  std::string key;
  std::string value;
  std::vector<std::string> records;
  // query se rank url count
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    if (key != last_key && !last_key.empty()) {
      proc(last_key, records, &parser, &url_norm, &url_class, task);
      last_key.clear();
      records.clear();
    }
    records.push_back(value);
    last_key = key;
  }
  if (key != last_key && !last_key.empty()) {
    proc(last_key, records, &parser, &url_norm, &url_class, task);
    last_key.clear();
    records.clear();
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

    QueryMetaInfo query_info;
    for (int i = 0; i < (int)top_n.size(); ++i) {
      MetaInfo* info = query_info.add_meta();
      const std::string& url = top_n[i].second;
      uint64 count = top_n[i].first;
      info->set_url(base::CalcUrlSign(url.c_str(), url.length()));
      if (count > (uint64)kInt32Max) count = kInt32Max;
      info->set_count(count);
    }
    // query se rank url count
    std::string dump;
    CHECK(query_info.SerializeToString(&dump));
    task->OutputWithFilePrefix(query, dump, se);
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url_query_count");
  if (base::mapreduce::IsMapApp()) {
    // CHECK(!FLAGS_history_data_path.empty() || !FLAGS_pv_log_path.empty()) 
    //   << "specify history data path and pv_log path";
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}

