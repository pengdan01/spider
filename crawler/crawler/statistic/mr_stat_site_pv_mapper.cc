#include <iostream>
#include <map>
#include <string>

#include "base/common/base.h"
#include "base/mapreduce/mapreduce.h"
#include "base/common/logging.h"
#include "base/encoding/url_encode.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/url.h"

DEFINE_int32(url_field_id, 0, "start from 0, will stat url in this field");

const uint64 kLocalCombineMaxSize = 100000;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::string url;
  std::vector<std::string> tokens;
  std::map<std::string, int64> local_combiner;

  CHECK_GE(FLAGS_url_field_id, 0);

  const std::string &file_path = task->GetInputSplit().hdfs_path;
  CHECK(!file_path.empty());
  bool from_history_stat = false;
  if (base::MatchPattern(file_path, "*/stat/*")) {
    from_history_stat = true;
  }
  // PV log 数据，记录格式如下：
  // agentid \t timestamp \t url \t refer_url \t refer_query \t refer_search_engine \t refer_anchor
  // \t url_attr \t entrance_id \t duration
  // 具体各个字段含义，请参考：http://192.168.11.60:8081/pages/viewpage.action?pageId=1671227
  while (task->NextKeyValue(&key, &value)) {
    if (from_history_stat) {
      task->Output(key, value);
      continue;
    }
    size_t num;
    tokens.clear();
    base::SplitString(key + "\t" + value, "\t", &tokens);
    num = tokens.size();
    LOG_IF(FATAL, (int)num <= FLAGS_url_field_id) << "num <= FLAGS_url_field_id, record: " << key+"\t"+value;
    url = tokens[FLAGS_url_field_id];
    base::TrimWhitespaces(&url);
    if (url.empty()) {
      LOG(ERROR) << "Ingore invalid input: " << key + "\t" + value;
      continue;
    }
    if (!web::has_scheme(url)) {
      url = "http://" + url;
    }
    GURL gurl(url);
    if (!gurl.is_valid()) {
      LOG(ERROR) << "Ingore invalid input: " << key + "\t" + value;
      continue;
    }
    // 类似 combiner 的功能
    const std::string &host = gurl.host();
    local_combiner[host] += 1;
    if (local_combiner.size() > kLocalCombineMaxSize) {
      for (auto it = local_combiner.begin(); it != local_combiner.end(); ++it) {
        std::string out = base::StringPrintf("%ld", it->second);
        task->Output(it->first, out);
      }
      local_combiner.clear();
    }
  }
  // Out all the data buffered in local combine structure
  for (auto it = local_combiner.begin(); it != local_combiner.end(); ++it) {
    std::string out = base::StringPrintf("%ld", it->second);
    task->Output(it->first, out);
  }
  local_combiner.clear();
  return 0;
}
