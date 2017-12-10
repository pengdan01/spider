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

#include "crawler/fetcher2/fetcher.h"
#include "crawler/fetcher2/data_io.h"

#include "web/url/gurl.h"

#include "log_analysis/common/log_common.h"

static void Reducer();
static void FetchedLog();

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "uv_data");

  if (base::mapreduce::IsMapApp()) {
    FetchedLog();
  } else {
    CHECK(base::mapreduce::IsReduceApp());
    Reducer();
  }

  return 0;
}

// output:
// url \t num \t SUCCESS/FAIL
static void FetchedLog() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::map<std::string, int64> host_success;
  std::map<std::string, int64> host_fail;

  std::string key, value;
  crawler2::UrlFetched url_fetched;
  while (task->NextKeyValue(&key, &value)) {
    //////////////  tricky start ///////////////////////////
    // XXX:(huangboxiang) 爬虫生成的数据有field num不对的问题,在该问题解决之前
    // 先用 tricky 的手段过滤
    std::vector<std::string> tricky_tokens;
    std::string tricky_string = key + "\t" + value;
    base::SplitString(tricky_string, "\t", &tricky_tokens);
    if (tricky_tokens.size() != 18) {
      LOG(INFO) << "!=18";
      task->ReportAbnormalData("!=18", key + "\t" + value, "");
      continue;
    }
    //////////////  tricky end  //////////////////////////
    crawler2::StringToUrlFetched(key + "\t" + value, &url_fetched);
    const std::string& url = url_fetched.url_to_fetch.url;
    bool success = url_fetched.success;
    GURL gurl(url);
    if (!gurl.has_host()) {
      LOG_EVERY_N(INFO, 100) << "no host";
      task->ReportAbnormalData("no host", key + "\t" + value, "");
      continue;
    }
    std::string host = gurl.host();
    if (success) {
      host_success[host] += 1;
    } else {
      host_fail[host] += 1;
    }

    if (host_success.size() > 1000000u) {
      for (auto it = host_success.begin(); it != host_success.end(); it++) {
        task->Output(it->first, base::StringPrintf("%ld\tSUCCESS", it->second));
      }
      host_success.clear();
    }
    if (host_fail.size() > 1000000u) {
      for (auto it = host_fail.begin(); it != host_fail.end(); it++) {
        task->Output(it->first, base::StringPrintf("%ld\tFAIL", it->second));
      }
      host_fail.clear();
    }
  }
  for (auto it = host_success.begin(); it != host_success.end(); it++) {
    task->Output(it->first, base::StringPrintf("%ld\tSUCCESS", it->second));
  }
  for (auto it = host_fail.begin(); it != host_fail.end(); it++) {
    task->Output(it->first, base::StringPrintf("%ld\tFAIL", it->second));
  }
}

// output:
// host \t total_fetched \t success \t fail \t fail_ratio
static void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string host;
  std::vector<std::string> tokens;
  while (task->NextKey(&host)) {

    int64 total_fetched = 0;
    int64 success = 0;
    int64 fail = 0;
    std::string value;
    while (task->NextValue(&value)) {
      LOG_EVERY_N(INFO, 10000) << value;
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      CHECK_EQ(tokens.size(), 2u) << value;
      int64 num = base::ParseInt64OrDie(tokens[0]);
      total_fetched += num;
      if (tokens[1] == "SUCCESS") {
        success += num;
      } else {
        CHECK(tokens[1] == "FAIL") << value;
        fail += num;
      }
    }
    CHECK_GT(total_fetched, 0) << host;
    task->Output(host, base::StringPrintf("%ld\t%ld\t%ld\t%g", total_fetched,
                                          success, fail, ((double)fail)/(double)total_fetched));
  }
}

