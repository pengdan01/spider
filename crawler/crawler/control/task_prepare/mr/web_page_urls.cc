#include <vector>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
// #include "web/url_norm/url_norm.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
static void parse_web_page_db_proto();
static void dedup_source_url();

// 从网页库中分析出 source url, 并去重
int main(int argc, char** argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");

  if (base::mapreduce::IsMapApp()) {
    parse_web_page_db_proto();
  } else {
    CHECK(base::mapreduce::IsReduceApp());
    dedup_source_url();
  }
}

static void parse_web_page_db_proto() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  // web::UrlNorm url_norm;
  crawler2::Resource resource;
  std::string key, value;
  while (task->NextKeyValue(&key, &value)) {
    resource.Clear();
    CHECK(crawler2::HbaseDumpToResource(value.data(), value.size(), &resource));
    CHECK(resource.has_brief());
    const crawler2::Brief& brief = resource.brief();

    const std::string& source_url = brief.source_url();
    // XXX(huangboxiang): 不过滤, 因为无法有效区分 404 是什么原因造成的
//     int64 response_code = brief.crawl_info().response_code();
//     if (response_code != 200) {
//       LOG_EVERY_N(INFO, 10000) << "Abandon response_code: " << response_code
//                                << source_url;
//       continue;
//     }
    std::string normed_url = source_url;
    // XXX(huangboxiang): 不归一化
//     if (!url_norm.NormalizeClickUrl(source_url, &normed_url)) {
//       LOG(ERROR) << "Invalid url: " << source_url;
//       continue;
//     }
    LOG_EVERY_N(INFO, 100000) << "output url: " << normed_url;
    task->Output(normed_url, "");
  }
}

static void dedup_source_url(){
  base::mapreduce::ReduceTask* task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string url;
  while (task->NextKey(&url)) {
    task->Output(url, "");
  }
}
