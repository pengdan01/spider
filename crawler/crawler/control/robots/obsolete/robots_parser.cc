#include <vector>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_util.h"
#include "web/url/gurl.h"
#include "base/zip/snappy.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/proto2/proto_transcode.h"

DEFINE_string(webdb_prefix, "/user/pengdan/webdb_url", "webdb prefix");
DEFINE_string(robots_prefix, "/user/crawler/special_crawler_individual", "robots prefix");

static void Webdb();
static void Robots();
static void Reducer();

std::string simplify_hdfs_path(const std::string& hdfs_path) {
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

bool is_file_in(const std::string& prefix, const std::string& filename) {
  std::string s = simplify_hdfs_path(prefix);
  std::string t = simplify_hdfs_path(filename);
  if (base::StartsWith(t, s, false)) {
    return true;
  }
  return false;
}


// 分析爬虫的抓取结果, 获取 raw content(robots)
// 分析 host 的所有 url 是否符合 robots 协议

int main(int argc, char** argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");

  if (base::mapreduce::IsMapApp()) {
    const base::mapreduce::InputSplit split = base::mapreduce::GetMapTask()->GetInputSplit();

    const std::string file_name = simplify_hdfs_path(split.hdfs_path);
    LOG(INFO) << "simplified mapper input file: " << file_name;
    if (is_file_in(FLAGS_robots_prefix, file_name)) {
      Robots();
    } else if (is_file_in(FLAGS_webdb_prefix, file_name)) {
      Webdb();
    } else {
      LOG(FATAL) << "invalid mapper";
    }
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    LOG(FATAL) << "no type app";
  }
  return 0;
}

static void Robots() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();

  std::string key, value;
  while (task->NextKeyValue(&key, &value)) {
    crawler2::Resource resource;
    CHECK(crawler2::HbaseDumpToResource(value.data(), value.size(), &resource));
    CHECK(resource.has_brief());
    CHECK(resource.brief().has_crawl_info());
    CHECK(resource.brief().crawl_info().has_response_code());
    const std::string& url = resource.brief().source_url();
    GURL gurl(url);
    if (!gurl.has_host()) {
      continue;
    }
    int response_code = resource.brief().crawl_info().response_code();
    if (response_code != 200) {
      continue;
    }
    CHECK(resource.has_content());
    if (!resource.content().has_snappy_raw_content()) {
      continue;
    }
    CHECK(resource.content().has_snappy_raw_content());
    std::string uncompressed;
    const std::string& compressed = resource.content().snappy_raw_content();
    CHECK(base::snappy::Uncompress(compressed.data(), compressed.size(), &uncompressed));
    if (uncompressed.empty()) {
      continue;
    }
    std::string utf8_content;
    int skipped_bytes, nonascii_bytes;
    if (!base::ConvertHTMLToUTF8WithBestEffort(url, resource.brief().response_header(),
                                          uncompressed, &utf8_content,
                                          &skipped_bytes, &nonascii_bytes)) {
      LOG(ERROR) << "failed to convert to utf8" << url;
      continue;
    }
    std::string line_escaped_content;
    base::LineEscape(uncompressed, &line_escaped_content);
    if (line_escaped_content.size() > 100*1024 ||
        line_escaped_content.find("User-agent") == std::string::npos) {
      continue;
    }

    const std::string& host = gurl.host();
    uint64 sign = base::CityHash64(host.data(), host.size());
    int32 bucket = sign % task->GetReducerNum();
    LOG_EVERY_N(INFO, 100) << url << ", " << bucket << ", " << line_escaped_content;
    task->OutputWithReducerID(host + "\tA", line_escaped_content, bucket);
  }
}

// url \t title
static void Webdb() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();
  std::string key, value;
  while (task->NextKeyValue(&key, &value)) {
    const std::string& url = key;
    GURL gurl(url);
    if (!gurl.is_valid() || !gurl.has_host()) {
      continue;
    }
    const std::string host = gurl.host();
    uint64 sign = base::CityHash64(host.data(), host.size());
    int32 bucket = sign % task->GetReducerNum();
    //task->OutputWithReducerID(host + "\tB", key + "\t" + value, bucket);
    task->OutputWithReducerID(host + "\tB", key, bucket);
  }
}

static void Reducer() {
  base::mapreduce::ReduceTask* task = base::mapreduce::GetReduceTask();

  std::string key;
  while (task->NextKey(&key)) {
    std::string value;
    while (task->NextValue(&value)) {
      task->Output(key, value);
    }
  }
}
