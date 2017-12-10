#include <vector>
#include <set>
#include <fstream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/random/pseudo_random.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "web/url/gurl.h"
#include "base/zip/snappy.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/proto2/proto_transcode.h"

DEFINE_string(bots_prefix, "/user/crawler/robots/", "robots 协议目录前缀");
DEFINE_int32(url_field, 0, "待查 url 数据中, url 所在字段, 字段间以 TAB 分割");
DEFINE_string(big_family_host, "./big_family_host.txt", "含有大量 url 的 host, 分桶不均的潜在因素");

void LoadBigFamilyHost(std::set<std::string>* big_family_hosts) {
  big_family_hosts->clear();
  std::ifstream is(FLAGS_big_family_host);
  CHECK(is.good()) << FLAGS_big_family_host;
  std::string line;
  std::vector<std::string> flds;
  while (getline(is, line)) {
    if (line.empty()) {
      continue;
    }
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 2u) << line;
    big_family_hosts->insert(flds[0]);
  }
  is.close();
}

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

// key: host
// value: robots
void Robots() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();

  std::set<std::string> big_family_hosts;
  LoadBigFamilyHost(&big_family_hosts);

  std::string key, value;
  while (task->NextKeyValue(&key, &value)) {
    const std::string& host = key;
    if (value.find("User-agent") == std::string::npos &&
        value.find("user-agent") == std::string::npos) {
      continue;
    }
    std::string escaped_content;
    base::LineEscape(value, &escaped_content);
    if (big_family_hosts.find(host) == big_family_hosts.end()) {
      uint64 sign = base::CityHash64(host.data(), host.size());
      int32 bucket = sign % task->GetReducerNum();
      LOG_EVERY_N(INFO, 10000) << host << ", " << bucket << ", " << escaped_content;
      task->OutputWithReducerID(host + "\tA", escaped_content, bucket);
    } else {
      for (int i = 0; i < task->GetReducerNum(); ++i) {
        LOG_EVERY_N(INFO, 10000) << host << ", " << i << ", " << escaped_content;
        task->OutputWithReducerID(host + "\tA", escaped_content, i);
      }
    }
  }
}

// 输入必须是 streaming-text 格式
// 第一个 TAB 之前的部分会作为 key
void Urls() {
  base::mapreduce::MapTask* task = base::mapreduce::GetMapTask();

  std::set<std::string> big_family_hosts;
  LoadBigFamilyHost(&big_family_hosts);
  base::PseudoRandom random;

  std::string key, value;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    std::string record;
    if (value != "") {
      record = key + "\t" + value;
    } else {
      record = key;
    }
    flds.clear();
    base::SplitString(record, "\t", &flds);
    CHECK_GT((int32)flds.size(), FLAGS_url_field);
    const std::string& url = flds[FLAGS_url_field];
    GURL gurl(url);
    if (!gurl.is_valid() || !gurl.has_host()) {
      continue;
    }
    const std::string& host = gurl.host();
    if (big_family_hosts.find(host) == big_family_hosts.end()) {
      uint64 sign = base::CityHash64(host.data(), host.size());
      int32 bucket = sign % task->GetReducerNum();
      LOG_EVERY_N(INFO, 10000) << host << ", " << bucket << ", " << record;
      task->OutputWithReducerID(host + "\tB", record, bucket);
    } else {
      int32 bucket = random.GetInt(0, task->GetReducerNum()-1);
      LOG_EVERY_N(INFO, 10000) << host << ", " << bucket << ", " << record;
      task->OutputWithReducerID(host + "\tB", record, bucket);
    }
  }
}

void Reducer() {
  base::mapreduce::ReduceTask* task = base::mapreduce::GetReduceTask();

  std::string key;
  while (task->NextKey(&key)) {
    std::string value;
    while (task->NextValue(&value)) {
      task->Output(key, value);
    }
  }
}

int main(int argc, char** argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");

  if (base::mapreduce::IsMapApp()) {
    const base::mapreduce::InputSplit split = base::mapreduce::GetMapTask()->GetInputSplit();

    const std::string file_name = simplify_hdfs_path(split.hdfs_path);
    LOG(INFO) << "simplified mapper input file: " << file_name;
    if (is_file_in(FLAGS_bots_prefix, file_name)) {
      Robots();
    } else {
      Urls();
    }
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    LOG(FATAL) << "no type app";
  }
  return 0;
}
