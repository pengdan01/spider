#include <unordered_map>
#include <string>
#include <vector>
#include <map>

#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
// #include "base/hash_function/city.h"
#include "base/file/file_path.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

// #include "crawler/exchange/task_data.h"
// #include "crawler/api/base.h"
// #include "crawler/fetcher2/fetcher.h"
// #include "crawler/fetcher2/data_io.h"
// #include "web/url_category/url_categorizer.h"
// 
// #include "log_analysis/common/log_common.h"
// #include "log_analysis/common/url_util.h"

static void MapperOutput(std::unordered_map<std::string, double>* dict,
                         base::mapreduce::MapTask* task);
static void LogProc(int32 level);
static void Reducer();
static bool is_file_in(const std::string& prefix, const std::string& filename);
static std::string simplify_hdfs_path(const std::string&);

// for reducer
DEFINE_double(uv_lowerbound, 30, "当 URL 的 UV 数量超过 (>) uv_lowerbound 的时候， URL 才会作为候选集去查 robots");
DEFINE_int32(path_depth, 3, "选取的数据路径最大深度");
// for mapper
DEFINE_string(uv_rank_prefix, "/app/feature/tasks/page_analyze/data/uv_rank/", "pv log path prefix");
DEFINE_string(show_rank_prefix, "/app/feature/tasks/page_analyze/data/show_rank/", "search log path prefix");

static const int kMaxMapSize = 1200000;

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "收集 robots 禁止的 url");

  if (base::mapreduce::IsMapApp()) {
    const base::mapreduce::InputSplit split = base::mapreduce::GetMapTask()->GetInputSplit();
    const std::string file_name = simplify_hdfs_path(split.hdfs_path);

    LOG(INFO) << "uv rank prefix: " << FLAGS_uv_rank_prefix;
    LOG(INFO) << "show rank prefix: " << FLAGS_show_rank_prefix;
    LOG(INFO) << "simplified mapper input file: " << file_name;
    if (is_file_in(FLAGS_uv_rank_prefix, file_name)) {
      LOG(INFO) << file_name << " is UvRank";
      LogProc(4);
    } else if (is_file_in(FLAGS_show_rank_prefix, file_name)){
      LOG(INFO) << file_name << " is ShowRank";
      if (file_name.find("baidu") != std::string::npos) {
        LOG(INFO) << "baidu";
        LogProc(2);
      } else if (file_name.find("google") != std::string::npos) {
        LOG(INFO) << "google";
        LogProc(3);
      } else {
        LOG(ERROR) << "Neither baidu nor google, skip data!";
      }
    } else {
      LOG(FATAL) << file_name << " type unknown";
    }
  } else {
    CHECK(base::mapreduce::IsReduceApp());
    Reducer();
  }

  return 0;
}

// get uv from show rank
//
// output:
// url \t level \t score
static void LogProc(int32 level) {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();

  std::unordered_map<std::string, double> combine_url;

  std::string key, value;
  std::vector<std::string> tokens;
  while (task->NextKeyValue(&key, &value)) {
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    if (tokens.size() != 1u) {
      LOG(ERROR) << "tokens.size() != 3u, " << key + "\t" + value;
      continue;
    }
    const std::string& url = key;
    double score;
    if (!base::StringToDouble(value, &score)) {
      LOG(ERROR) << "value not a double, " << key + "\t" + value;
      continue;
    }
    tokens.clear();
    base::SplitStringWithOptions(url, "/", true, true, &tokens);
    GURL gurl(url);
    if (!gurl.is_valid() || !gurl.has_host()) {
      continue;
    }
    if ((int32)tokens.size() >= FLAGS_path_depth + 3) {
      LOG_EVERY_N(INFO, 10000) << "tokens size " << tokens.size()
                               << " >= " << FLAGS_path_depth + 3 << ", Skip, " << url;
      continue;
    } else if (level > 3 && (score < FLAGS_uv_lowerbound || gurl.has_query())) {
      LOG_EVERY_N(INFO, 10000) << "UV skip " << score << ", " << gurl.has_query() << ", " << url;
      continue;
    }
    // 肯定可以抓取的内容无需输出
    if (base::StartsWith(url, "http://zhidao.baidu.com", false) ||
        base::StartsWith(url, "http://baike.baidu.com", false) ||
        base::StartsWith(url, "http://wenku.baidu.com", false) ||
        base::StartsWith(url, "http://www.baidu.com", false) ||
        base::StartsWith(url, "http://wenwen.soso.com", false) ||
        base::StartsWith(url, "http://tieba.baidu.com", false) ||
        base::StartsWith(url, "http://www.19lou.com", false) ||
        base::StartsWith(url, "http://dict.youdao.com", false) ||
        base::StartsWith(url, "http://iask.sina.com.cn", false) ||
        base::StartsWith(url, "http://www.smarter.com.cn", false) ||
        base::StartsWith(url, "http://finance.qq.com", false)) {
      continue;
    }

    std::string dict_key = base::StringPrintf("%s\t%d", url.c_str(), level);
    combine_url[dict_key] += score;
    if ((int32)combine_url.size() > kMaxMapSize) {
      MapperOutput(&combine_url, task);
    }
  }  // end while
  MapperOutput(&combine_url, task);
}

static void MapperOutput(std::unordered_map<std::string, double>* dict,
                         base::mapreduce::MapTask* task) {
  LOG(INFO) << "dict size: " << dict->size() << ", dump dict";
  std::vector<std::string> flds;
  for (auto it = dict->begin(); it != dict->end(); ++it) {
    flds.clear();
    base::SplitString(it->first, "\t", &flds);
    CHECK_EQ(flds.size(), 2u) << it->first;
    task->Output(flds[0], flds[1] + "\t" + base::StringPrintf("%g", it->second));
    LOG_EVERY_N(INFO, 100000) << "Output: "
                              << flds[0] + "\t"
                              << flds[1] + "\t" + base::StringPrintf("%g", it->second);
  }
  dict->clear();
  std::unordered_map<std::string, double> tmp;
  dict->swap(tmp);
}

// input:
// url \t level \t score
//
// output
// url \t level \t score
static void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string url;
  std::vector<std::string> tokens;
  std::map<int32, double> level_dict;
  while (task->NextKey(&url)) {
    std::string value;
    level_dict.clear();
    while (task->NextValue(&value)) {
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      CHECK_EQ(tokens.size(), 2u) << value;
      int32 level = base::ParseInt32OrDie(tokens[0]);
      double score = base::ParseDoubleOrDie(tokens[1]);
      level_dict[level] = score;
    }
    int32 level = -1;
    double score = 0;
    // 命中百度 show rank
    if (level_dict.find(2) != level_dict.end()) {
      level = 2;
      score = level_dict[2];
    }
    // 命中google show rank
    if (level_dict.find(3) != level_dict.end()) {
      if (level == 2) {
        level = 1;
      } else {
        level = 3;
        score = level_dict[3];
      }
    }
    // 只命中 uv rank
    if (level < 0 && level_dict.find(4) != level_dict.end()) {
      level = 4;
      score = level_dict[4];
    }
    if (level > 0) {
      task->Output(url, base::StringPrintf("%d\t%g", level, score));
      LOG_EVERY_N(INFO, 100000) << "Output: " << url + "\t"
                                << base::StringPrintf("%d\t%g", level, score);
    } else {
      LOG(FATAL) << "Invalid level: " << url + "\t" + value;
    }
  }
}

static std::string simplify_hdfs_path(const std::string& hdfs_path) {
  uint64 pos = 0;
  int64 len = hdfs_path.size();
  // 去除 header
  if (base::StartsWith(hdfs_path, "hdfs://", false)) {
    pos = hdfs_path.find_first_of('/', 7);
    CHECK_NE(pos, std::string::npos);
    len -= pos;
  }
  // 去除末尾的 '/'
  for (auto pt = hdfs_path.data() + hdfs_path.size() - 1;
       pt != hdfs_path.data() + pos && *pt == '/'; pt--) {
    len -= 1;
  }
  CHECK_GT(len, 0);
  
  return hdfs_path.substr(pos, len);
}

static bool is_file_in(const std::string& prefix, const std::string& filename) {
  std::string s = simplify_hdfs_path(prefix);
  if (base::StartsWith(filename, s, false)) {
    return true;
  }
  return false;
}

