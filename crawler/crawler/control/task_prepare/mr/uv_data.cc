#include <set>
#include <unordered_map>
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/city.h"
#include "base/file/file_path.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"

// #include "crawler/control/task_prepare/pv_log_parser.h"
// #include "crawler/control/task_prepare/url_rule.h"
#include "crawler/exchange/task_data.h"
#include "crawler/api/base.h"
#include "crawler/fetcher2/fetcher.h"
#include "crawler/fetcher2/data_io.h"
#include "web/url_category/url_categorizer.h"

#include "log_analysis/common/log_common.h"
#include "log_analysis/common/url_util.h"

// search log format:
//  #1     #2           #3        #4       #5         #6     #7
// sid \t agentid \t timestamp \t query \t se_name \t rank \t url
//
// pv log format:
//  #1          #2         #3        #4
// agentid \t  timestamp \t url  \t ref_url \t ...

static double ComputeUV(const std::string& url, const std::string& ref_url,
                        int32 url_id, const std::string& supplement,
                        std::set<std::string>* baidu_set, std::set<std::string>* google_set);
static void PvLog();
static void SearchLog();
static void Reducer();
static bool is_file_in(const std::string& prefix, const std::string& filename);
static std::string simplify_hdfs_path(const std::string&);
static void FetchedLog(bool use_failed_log, bool use_success_log);

// for reducer
DEFINE_double(uv_lowerbound, 1.99, "当 URL 的 UV 数量超过 (>) uv_lowerbound 的时候， URL 才会作为候选集加入到爬去列表当中.");
// for mapper
DEFINE_string(pv_log_prefix, "/app/user_data/pv_log/", "pv log path prefix");
DEFINE_string(search_log_prefix, "/app/user_data/search_log/", "search log path prefix");
DEFINE_string(fetched_log_prefix, "/user/crawler/fetched_status/daily/", "fetched status log path prefix");
DEFINE_string(use_faildata_date, "20120101", "指定日期的抓取失败的 url 会尝试抓取");
DEFINE_int32(max_try_times, 3, "url 重试次数超过 |max_try_times| 的时候, 丢弃该 url");

static const int kMaxMapSize = 1200000;

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "uv_data");

  if (base::mapreduce::IsMapApp()) {
    const base::mapreduce::InputSplit split = base::mapreduce::GetMapTask()->GetInputSplit();
    const std::string file_name = simplify_hdfs_path(split.hdfs_path);

    LOG(INFO) << "use_faildata_date: " << FLAGS_use_faildata_date;
    LOG(INFO) << "pv log prefix: " << FLAGS_pv_log_prefix;
    LOG(INFO) << "search log prefix: " << FLAGS_search_log_prefix;
    LOG(INFO) << "simplified mapper input file: " << file_name;
    if (is_file_in(FLAGS_pv_log_prefix, file_name)) {
      LOG(INFO) << file_name << " is PvLog";
      PvLog();
    } else if (is_file_in(FLAGS_search_log_prefix, file_name)){
      LOG(INFO) << file_name << " is SearchLog";
      SearchLog();
    } else if (is_file_in(FLAGS_fetched_log_prefix, file_name)){
      // some thing like this:
      // /user/crawler/general_crawler_output/20120626/20/status/2012062620.033.2.20120613050932_002_00033.status.st/2.status
      LOG(INFO) << file_name << " is FetchedLog";
      std::vector<std::string> flds;
      base::SplitStringWithOptions(file_name, "/", true, true, &flds);
      CHECK_GE(flds.size(), 5u);
      std::string date = flds[flds.size() - 5];
      std::vector<std::string> tokens;
      base::SplitString(flds[flds.size()-2], ".", &tokens);
      int priority = -1;
      if (!base::StringToInt(tokens[2], &priority)) {
        LOG(ERROR) << "Failed to Parse Dir" << file_name;
        return true;
      }
      CHECK_LE(priority, 5);
      CHECK_GE(priority, 0);
      LOG(INFO) << "priority: " << priority;
      LOG(INFO) << "date: " << date;

      bool use_failed_log = false;
      bool use_success_log = true;
//       if (priority == 4 || priority == 5) {
//         use_success_log = false;
//       }
      if (FLAGS_use_faildata_date == date) {
        use_failed_log = true;
      }
      if (use_success_log) LOG(INFO) << "use success log";
      if (use_failed_log) LOG(INFO) << "use failed log";
      FetchedLog(use_failed_log, use_success_log);
    } else {
      LOG(FATAL) << file_name << " type unknown";
    }
  } else {
    CHECK(base::mapreduce::IsReduceApp());
    LOG(INFO) << "max try times: " << FLAGS_max_try_times;
    Reducer();
  }

  return 0;
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

// get uv from search log
// input: search log
//  #1     #2           #3        #4       #5         #6     #7
// sid \t agentid \t timestamp \t query \t se_name \t rank \t url
//
// output:
// url \t uv \t file_type \t ref_url("") \t try_times(0) \t index_baidu(1/0) \t index_google(1/0)
static void SearchLog() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  // 存放一个用户 search query 对应 的 url
  std::unordered_map<std::string, double> user_url;
  // url_uv
  std::unordered_map<std::string, double> combine_url;

  std::set<std::string> baidu_set;
  std::set<std::string> google_set;

  web::util::UrlCategorizer url_categorizer;
  std::string prev_user;
  std::string key, value;
  std::vector<std::string> tokens;
  while (task->NextKeyValue(&key, &value)) {
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    CHECK_GE(tokens.size(), 6u);
    if (base::ParseInt64OrDie(tokens[4]) > 10) {
      continue;
    }
    const std::string& user = tokens[0];
    std::string click_url;
    if (!web::RawToClickUrl(tokens[5], NULL, &click_url)) {
      LOG(ERROR) << "Failed on raw2click transform, url:" << tokens[5];
      continue;
    }
    std::string search_engine = tokens[3];
    int32 url_id;
    std::string supplement_info;
    if (!url_categorizer.Category(click_url, &url_id, &supplement_info) ||
        url_id < web::util::kSplitLineForCrawler) {
      LOG_EVERY_N(INFO, 1000000) << "Abandon non-tidy url: "
                                 << click_url;
      continue;
    }

    std::string url = click_url;
    if (search_engine == log_analysis::kBaidu || search_engine == log_analysis::kGoogle) {
      if (search_engine == log_analysis::kBaidu) {
        if (!log_analysis::ParseBaiduTargetUrl(click_url, &url)) {
          continue;
        }
      } else {
        if (!log_analysis::ParseGoogleTargetUrl(click_url, &url)) {
          url = click_url;
        }
      }
      if (!url_categorizer.Category(url, &url_id, &supplement_info) ||
          url_id < web::util::kSplitLineForCrawler) {
        continue;
      }
      if (search_engine == log_analysis::kBaidu) {
        baidu_set.insert(url);
      } else {
        google_set.insert(url);
      }
    }

    if (user != prev_user) {
      for (auto it = user_url.begin(); it != user_url.end(); it++) {
        combine_url[it->first] += it->second;
      }
      user_url.clear();
      {
        std::unordered_map<std::string, double> tmp;
        tmp.swap(user_url);
      }
    }
    // (pengdan) 只要 在 search log 出现且 rank > 10 的都抓取
    // 同一个用户看了 100篇, 也只算 一个 uv
    // 前提: search log 数据按照 agent id 有序
    const double uv = 2.0;
    if (uv > user_url[url]) {
      user_url[url] = uv;
    }
    prev_user = user;

    if ((int)combine_url.size() > kMaxMapSize) {
      LOG(INFO) << "dict full, dump dict. dict size: " << combine_url.size();
      for (auto it = combine_url.begin(); it != combine_url.end(); it++) {
        bool is_indexed_baidu = false;
        bool is_indexed_google = false;
        if (baidu_set.find(it->first) != baidu_set.end()) {
          is_indexed_baidu = true;
        }
        if (google_set.find(it->first) != google_set.end()) {
          is_indexed_google = true;
        }
        LOG_EVERY_N(INFO, 200000) << "key: " << it->first
                                  << "value: " << base::StringPrintf("B\t%g\t%d\t\t0\t%d\t%d", it->second,
                                                                     crawler::kHTML, is_indexed_baidu, is_indexed_google);
        task->Output(it->first, base::StringPrintf("B\t%g\t%d\t\t0\t%d\t%d",
                                                   it->second, crawler::kHTML, is_indexed_baidu, is_indexed_google));
      }
      combine_url.clear();
      {
        std::unordered_map<std::string, double> tmp;
        tmp.swap(combine_url);
      }
      baidu_set.clear();
      google_set.clear();
    }

  }  // end while

  for (auto it = user_url.begin(); it != user_url.end(); it++) {
    combine_url[it->first] += it->second;
  }
  LOG(INFO) << "before termination, dump dict. dict size: " << combine_url.size();
  for (auto it = combine_url.begin(); it != combine_url.end(); it++) {
    bool is_indexed_baidu = false;
    bool is_indexed_google = false;
    if (baidu_set.find(it->first) != baidu_set.end()) {
      is_indexed_baidu = true;
    }
    if (google_set.find(it->first) != google_set.end()) {
      is_indexed_google = true;
    }
    LOG_EVERY_N(INFO, 20000) << "key: " << it->first
                             << "value: " << base::StringPrintf("B\t%g\t%d\t\t0\t%d\t%d", it->second, crawler::kHTML,
                                                                is_indexed_baidu, is_indexed_google);
    task->Output(it->first, base::StringPrintf("B\t%g\t%d\t\t0\t%d\t%d", it->second, crawler::kHTML,
                                               is_indexed_baidu, is_indexed_google));
  }
}

// get uv from pv log
// input: pv log
// userid \t timestamp \t url \t ref_url \t ...
//
// output:
// url \t uv \t file_type \t ref_url \t try_times(which === 0) \t is_indexed_baidu \t is_indexed_google
struct UrlAdditiveNode {
  double uv;
  std::string ref_url;
  explicit UrlAdditiveNode(double uv_, const std::string& ref_url_)
      : uv(uv_), ref_url(ref_url_) {}
  UrlAdditiveNode() : uv(0), ref_url("") {}
};
static void PvLog() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::unordered_map<std::string, UrlAdditiveNode> user_url;
  std::unordered_map<std::string, UrlAdditiveNode> combine_url;
  std::set<std::string> baidu_set;
  std::set<std::string> google_set;

  web::util::UrlCategorizer url_categorizer;
  std::string prev_user;
  std::vector<std::string> tokens;
  std::string key, value;
  while (task->NextKeyValue(&key, &value)) {
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    CHECK_GE(tokens.size(), 3u);

    const std::string& ref_url = tokens[2];
    const std::string& user = key;
    std::string tmp_url;
    if (!web::RawToClickUrl(tokens[1], NULL, &tmp_url)) {
      LOG(ERROR) << "Failed on raw2click transform, url:" << tokens[1];
      continue;
    }

    int32 url_id;
    std::string supplement_info;
    if (!url_categorizer.Category(tmp_url, &url_id, &supplement_info)) {
      LOG(ERROR) << "Invalid url: " << tmp_url;
      continue;
    }
    if (url_id < web::util::kSplitLineForCrawler) {
      LOG_EVERY_N(INFO, 1000000) << "Abandon non-tidy url: "
                                 << tmp_url;
      continue;
    }
    std::string url = tmp_url;
    if (url_id == web::util::kGoogleSearchResultClick ||
        url_id == web::util::kBaiduSearchResultClick) {
      if (url_id == web::util::kGoogleSearchResultClick) {
        if (!log_analysis::ParseGoogleTargetUrl(tmp_url, &url)) {
          url = tmp_url;
        }
      } else if (url_id == web::util::kBaiduSearchResultClick) {
        if (!log_analysis::ParseBaiduTargetUrl(tmp_url, &url)) {
          continue;
        }
      }
      if (!url_categorizer.Category(url, &url_id, &supplement_info) ||
          url_id < web::util::kSplitLineForCrawler) {
        continue;
      }
      LOG_EVERY_N(INFO, 1000) << "baidu/google click: " << url  << ", " << tmp_url;
    }

    if (user != prev_user) {
      if (user_url.size() < 80000) {
        for (auto it = user_url.begin(); it != user_url.end(); it++) {
          combine_url[it->first].uv += it->second.uv;
          if (it->second.ref_url != "") {
            combine_url[it->first].ref_url = it->second.ref_url;
          }
        }
      } else {
        LOG(ERROR) << "Abandon abnormal user: id = " << prev_user
                   << ", unique url num = " << user_url.size();
      }
      user_url.clear();
      {
        std::unordered_map<std::string, UrlAdditiveNode> tmp;
        tmp.swap(user_url);
      }
    }
    double uv = ComputeUV(url, ref_url, url_id, supplement_info, &baidu_set, &google_set);
    if (uv > user_url[url].uv) {
      user_url[url].uv = uv;
      if (ref_url != "") {
        user_url[url].ref_url = ref_url;
      }
    }
    if (user_url[url].ref_url == "" && ref_url != "") {
      user_url[url].ref_url = ref_url;
    }
    
    prev_user = user;

    if ((int)combine_url.size() > kMaxMapSize) {
      LOG(INFO) << "dict full, dump dict. dict size: " << combine_url.size();
      for (auto it = combine_url.begin(); it != combine_url.end(); it++) {
        bool is_indexed_baidu = false;
        bool is_indexed_google = false;
        if (baidu_set.find(it->first) != baidu_set.end()) {
          is_indexed_baidu = true;
        }
        if (google_set.find(it->first) != google_set.end()) {
          is_indexed_google = true;
        }
        LOG_EVERY_N(INFO, 200000) << "key: " << it->first
                                 << "value: " << base::StringPrintf("B\t%g\t%d\t%s\t0\t%d\t%d", it->second.uv,
                                                                    crawler::kHTML, it->second.ref_url.c_str(),
                                                                    is_indexed_baidu, is_indexed_google);
        task->Output(it->first,
                     base::StringPrintf("B\t%g\t%d\t%s\t0\t%d\t%d", it->second.uv, crawler::kHTML, it->second.ref_url.c_str(),
                                        is_indexed_baidu, is_indexed_google));
      }
      combine_url.clear();
      {
        std::unordered_map<std::string, UrlAdditiveNode> tmp;
        tmp.swap(combine_url);
      }
      baidu_set.clear();
      google_set.clear();
    }
  }  // end while

  for (auto it = user_url.begin(); it != user_url.end(); it++) {
    combine_url[it->first].uv += it->second.uv;
    if (it->second.ref_url != "") {
      combine_url[it->first].ref_url = it->second.ref_url;
    }
  }
  LOG(INFO) << "before termination, dump dict. dict size: " << combine_url.size();
  for (auto it = combine_url.begin(); it != combine_url.end(); it++) {
    bool is_indexed_baidu = false;
    bool is_indexed_google = false;
    if (baidu_set.find(it->first) != baidu_set.end()) {
      is_indexed_baidu = true;
    }
    if (google_set.find(it->first) != google_set.end()) {
      is_indexed_google = true;
    }
    LOG_EVERY_N(INFO, 200000) << "key: " << it->first
                              << "value: " << base::StringPrintf("B\t%g\t%d\t%s\t0\t%d\t%d", it->second.uv,
                                                                 crawler::kHTML, it->second.ref_url.c_str(),
                                                                 is_indexed_baidu, is_indexed_google);
    task->Output(it->first,
                 base::StringPrintf("B\t%g\t%d\t%s\t0\t%d\t%d", it->second.uv, crawler::kHTML, it->second.ref_url.c_str(),
                                    is_indexed_baidu, is_indexed_google));
  }
}

// output:
// url \t uv \t file_type \t ref_url
static void FetchedLog(bool use_failed_log, bool use_success_log) {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key, value;
  crawler2::UrlFetched url_fetched;
  while (task->NextKeyValue(&key, &value)) {
    //////////////  tricky start ///////////////////////////
    // XXX:(huangboxiang) 爬虫生成的数据有field num不对的问题,在该问题解决之前
    // 先用 tricky 的手段过滤
    std::vector<std::string> tricky_tokens;
    std::string tricky_string = key + "\t" + value;
    base::SplitString(tricky_string, "\t", &tricky_tokens);
    if (tricky_tokens.size() < 18 || tricky_tokens.size() > 19) {
      continue;
    }
    //////////////  tricky end  //////////////////////////
    crawler2::StringToUrlFetched(key + "\t" + value, &url_fetched);
    const std::string& url = url_fetched.url_to_fetch.url;
    bool success = url_fetched.success;
    if (!use_success_log && success) continue;
    if (!use_failed_log && !success) continue;
    if (use_success_log && success) {
      task->Output(url, "A");
      LOG_EVERY_N(INFO, 5000) << "success log, for barrior";
      continue;
    }
    if (use_failed_log && !success) {
      int try_times = url_fetched.url_to_fetch.tried_times + 1;
      CHECK_GE(try_times, 1) << key + "\t" + value;
      double uv = url_fetched.url_to_fetch.importance;
      int filetype = url_fetched.url_to_fetch.resource_type;
      CHECK_EQ(filetype, crawler::kHTML);
      const std::string& ref_url = url_fetched.url_to_fetch.referer;
      task->Output(url, base::StringPrintf("B\t%g\t%d\t%s\t%d\t0\t0",
                                           uv, filetype, ref_url.c_str(),
                                           try_times
                                          ));
      continue;
    }

    LOG(FATAL) << "case impossible";
  }
}

// input:
// url \t B \t uv \t file_type \t ref_url \t try_times \t indexed_baidu \t indexed_google
static void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string url;
  std::vector<std::string> tokens;
  while (task->NextKey(&url)) {
    std::string ref_url;
    double sum_uv = 0.0;
    std::string value;
    bool output = true;
    int try_times= 0;
    bool is_indexed_baidu = false, is_indexed_google = false;
    while (task->NextValue(&value)) {
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      CHECK_GE(tokens.size(), 1u) << value;
      CHECK(tokens[0] == "A" || tokens[0] == "B") << value;
      if (tokens[0] == "A") {
        output = false;
        break;
      }
      // B
      CHECK_EQ(tokens.size(), 7u);
      const std::string& file_type = tokens[2];
      CHECK_EQ(base::ParseInt64OrDie(file_type), crawler::kHTML);
      sum_uv += base::ParseDoubleOrDie(tokens[1]);
      if (tokens[3] != "") {
        ref_url = tokens[3];
      }
      int tmptry = (int)base::ParseInt64OrDie(tokens[4]);
      if (tmptry > try_times) {
        try_times = tmptry;
      }
      bool tmp_baidu = (bool)base::ParseInt32OrDie(tokens[5]);
      if (tmp_baidu) {
        is_indexed_baidu = true;
      }
      bool tmp_google = (bool)base::ParseInt32OrDie(tokens[6]);
      if (tmp_google) {
        is_indexed_google = true;
      }
    }

    // XXX(huangboxiang): url ip resource_type importance referer use_proxy tried_times is_indexed_baidu is_indexed_google is_realtime_craw
    if (output && sum_uv > FLAGS_uv_lowerbound && try_times < FLAGS_max_try_times) {
      task->Output(base::StringPrintf("%g", sum_uv),
                   base::StringPrintf("%s\t\t%d\t%g\t%s\t\t%d\t%d\t%d\t0",
                                      url.c_str(), crawler::kHTML, sum_uv, ref_url.c_str(), try_times, is_indexed_baidu, is_indexed_google)
                  );
    } else if (try_times >= FLAGS_max_try_times) {
      task->ReportAbnormalData("exceed max try times, abandon url", url, "");
      LOG(WARNING) << "exceed max try times, abandon url: " << url;
    }
    if (!output) {
      LOG_EVERY_N(INFO, 1000) << url << " succeed crawled in the nearest several days, skip";
    }
  }
}

static double ComputeUV(const std::string& url, const std::string& ref_url,
                        int32 url_id, const std::string& supplement,
                        std::set<std::string>* baidu_set, std::set<std::string>* google_set) {

  // 来自特定搜索引擎的搜索结果
  std::string query, search_engine;
  if (log_analysis::IsGeneralSearchClick(url, ref_url, &query, &search_engine)) {
    if (search_engine == log_analysis::kBaidu
        || search_engine == log_analysis::kGoogle
        || search_engine == log_analysis::kBing) {
      LOG_EVERY_N(INFO, 100000) << "From specific se, weighted 10: "
                                << url << ", ref:" << ref_url;
      if (search_engine == log_analysis::kBaidu) {
        baidu_set->insert(url);
      } else if (search_engine == log_analysis::kGoogle) {
        google_set->insert(url);
      }
      return 10.0;
    } else {
      return 0.0;
    }
  }

  // wiki 加分
  if (url_id == web::util::kWiki) {
    return 5.0;
  }
  // 惩罚论坛, 博客类
  if (url_id == web::util::kForum || url_id == web::util::kBlog) {
    return 0.4;
  }
  // 惩罚淘宝商品
  if (url_id == web::util::kItemsSell && supplement == "Taobao") {
    return 0.0001;
  }

  GURL gurl(url);
  const std::string host = gurl.host();

  // tieba 的搜索结果和有效内容混合在一起, 单独处理
  // 惩罚 论坛,博客类
  if (base::EndsWith(host, "tieba.baidu.com", false)) {
    return 0.4;
  }

  // 普通浏览
  return 1.0;
};

