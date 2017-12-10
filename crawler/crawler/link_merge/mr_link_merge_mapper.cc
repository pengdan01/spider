#include <iostream>
#include <string>

#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/common/base.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/selector/crawler_selector_util.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"

const unsigned int kLinkbaseRecordFieldNumber = 15;
// Record format like: url \t cmd_type \t timestamp
const unsigned int kUpdateFieldNumber = 3;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "crawler_link_merge");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  web::UrlNorm url_norm;

  std::string key;
  std::string value;
  std::string reason;
  std::vector<std::string> tokens;
  int64 total = 0;
  int64 count = 0;
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    size_t num = tokens.size();
    if (num != kLinkbaseRecordFieldNumber - 1 && num != kUpdateFieldNumber - 1) {
      LOG(WARNING) << "Record format invalid, field# : " << num+1 << ", record: " << key + "\t" + value;
      continue;
    }
    ++total;
    // 使用严格的规则过滤掉一些无效的 url, 例如: 其它搜索引擎生成的结果
    // NOTE: 临时过渡用, 后期不需要使用之, 因为已经在 selector 中应用了该规则
    if (crawler::WillFilterAccordingRules(key, &reason, true)) {
      ++count;
      LOG_EVERY_N(WARNING, 200) << reason << ", count: " << google::COUNTER;
      continue;
    }
    if (!url_norm.NormalizeClickUrl(key, &key)) {
      LOG(ERROR) << "NormalizeClickUrl() fail, url: " << key;
      continue;
    }
    task->Output(key, value);
  }
  LOG(INFO) << "Total url#: "  << total << ", apply rules and filter url#: " << count;
  return 0;
}
