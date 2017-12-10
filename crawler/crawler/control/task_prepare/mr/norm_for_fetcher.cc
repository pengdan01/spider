#include <unordered_map>
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/city.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"

// #include "crawler/control/task_prepare/pv_log_parser.h"
#include "crawler/exchange/task_data.h"
#include "crawler/api/base.h"
#include "web/url_norm/url_norm.h"
#include "log_analysis/common/log_common.h"

// DEFINE_bool(norm_site_alias, true, "normalization for site alias");
// DEFINE_bool(norm_slash_dot, true, "normalization for successive slash and dot");
// DEFINE_bool(norm_redundant_param, true, "normalization redundant query parameters");

static void norm_url();
static void combine_and_reform();

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "uv_data_norm");

  if (base::mapreduce::IsMapApp()) {
    norm_url();
  } else {
    CHECK(base::mapreduce::IsReduceApp());
    combine_and_reform();
  }

  return 0;
}

static void norm_url() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  web::UrlNorm url_norm;
  std::string key, value;
  std::vector<std::string> tokens;
  while (task->NextKeyValue(&key, &value)) {
    double uv = base::ParseDoubleOrDie(key);
    LOG_EVERY_N(INFO, 100000) << uv << ", key: " << key
                              << ", value: " << value;

    tokens.clear();
    base::SplitStringWithMoreOptions(value, "\t", false, false, 1, &tokens);
    CHECK_EQ(tokens.size(), 2u);
    const std::string& url = tokens[0];
    std::string new_url;
    if (!url_norm.NormalizeClickUrl(url, &new_url)) {
      LOG(ERROR) << "fail during normalization"
                 << "url: " << url;
      task->ReportAbnormalData("norm failed", url, "");
      continue;
    }
  
    task->Output(new_url, key + "\t" + tokens[1]);
  }
}

static void combine_and_reform() {
  base::mapreduce::ReduceTask* task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string url;
  std::vector<std::string> tokens;
  while (task->NextKey(&url)) {
    double sum_uv = 0;
    std::string value;
    std::string output_key, output_value;
    int64 num = 0;
    while (task->NextValue(&value)) {
      tokens.clear();
      base::SplitStringWithMoreOptions(value, "\t", false, false, 1, &tokens);
      CHECK_EQ(tokens.size(), 2u);
      sum_uv += base::ParseDoubleOrDie(tokens[0]);
      if (output_value.empty()) {
        output_value = url + "\t" + tokens[1];
      }
      num++;
    }
    if (num > 0) {
      LOG_EVERY_N(INFO, 5) << "check url: " << url;
    }

    // XXX(huangboxiang): 不修改 importance 字段
    output_key = base::StringPrintf("%g", sum_uv);
    task->Output(output_key, output_value);
  }
}
